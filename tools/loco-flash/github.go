package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"
)

const defaultRepo = "loco-can/loco-can-software"

type Release struct {
	Tag         string
	Name        string
	Prerelease  bool
	PublishedAt time.Time
	Images      []Image
}

type Client struct {
	Repo    string
	Token   string
	BaseURL string
	HTTP    *http.Client
}

func (c *Client) base() string {
	if c.BaseURL != "" {
		return strings.TrimRight(c.BaseURL, "/")
	}
	return "https://api.github.com"
}

func (c *Client) http() *http.Client {
	if c.HTTP != nil {
		return c.HTTP
	}
	return &http.Client{Timeout: 60 * time.Second}
}

type apiRelease struct {
	TagName     string     `json:"tag_name"`
	Name        string     `json:"name"`
	Draft       bool       `json:"draft"`
	Prerelease  bool       `json:"prerelease"`
	PublishedAt time.Time  `json:"published_at"`
	Assets      []apiAsset `json:"assets"`
}

type apiAsset struct {
	Name               string `json:"name"`
	BrowserDownloadURL string `json:"browser_download_url"`
	Size               int64  `json:"size"`
	Digest             string `json:"digest"`
}

func parseRepo(s string) (string, error) {
	s = strings.TrimSpace(s)
	s = strings.TrimSuffix(s, "/")
	s = strings.TrimSuffix(s, ".git")
	if i := strings.Index(s, "github.com/"); i >= 0 {
		s = s[i+len("github.com/"):]
	}
	s = strings.TrimPrefix(s, "https://")
	s = strings.TrimPrefix(s, "http://")
	parts := strings.Split(s, "/")
	if len(parts) != 2 || parts[0] == "" || parts[1] == "" {
		return "", fmt.Errorf("repo %q must be owner/name", s)
	}
	return parts[0] + "/" + parts[1], nil
}

func (c *Client) Releases(ctx context.Context, tag string, stableOnly bool) ([]Release, error) {
	repo, err := parseRepo(c.Repo)
	if err != nil {
		return nil, err
	}
	var raw []apiRelease
	if tag != "" {
		var one apiRelease
		if err := c.getJSON(ctx, c.base()+"/repos/"+repo+"/releases/tags/"+tag, &one); err != nil {
			return nil, err
		}
		raw = []apiRelease{one}
	} else {
		if err := c.getJSON(ctx, c.base()+"/repos/"+repo+"/releases?per_page=100", &raw); err != nil {
			return nil, err
		}
	}
	sort.SliceStable(raw, func(i, j int) bool {
		return raw[i].PublishedAt.After(raw[j].PublishedAt)
	})

	var out []Release
	for _, rel := range raw {
		if rel.Draft || (stableOnly && rel.Prerelease) {
			continue
		}
		images, err := c.images(ctx, rel)
		if err != nil {
			return nil, fmt.Errorf("release %s: %w", rel.TagName, err)
		}
		if len(images) == 0 {
			continue
		}
		out = append(out, Release{
			Tag:         rel.TagName,
			Name:        rel.Name,
			Prerelease:  rel.Prerelease,
			PublishedAt: rel.PublishedAt,
			Images:      images,
		})
	}
	if len(out) == 0 {
		if tag != "" {
			return nil, fmt.Errorf("release %s has no firmware images", tag)
		}
		return nil, fmt.Errorf("no firmware images in GitHub releases for %s", repo)
	}
	return out, nil
}

func (c *Client) images(ctx context.Context, rel apiRelease) ([]Image, error) {
	assets := map[string]apiAsset{}
	var manifest *apiAsset
	for _, asset := range rel.Assets {
		assets[asset.Name] = asset
		if asset.Name == "manifest.json" {
			copy := asset
			manifest = &copy
		}
	}
	if manifest != nil {
		body, err := c.get(ctx, manifest.BrowserDownloadURL)
		if err != nil {
			return nil, err
		}
		defer body.Close()
		list, err := readManifest(body)
		if err != nil {
			return nil, err
		}
		for i := range list {
			asset, ok := assets[list[i].File]
			if !ok {
				return nil, fmt.Errorf("manifest lists %s, release has no such file", list[i].File)
			}
			list[i].URL = asset.BrowserDownloadURL
			list[i].Size = asset.Size
			if list[i].SHA256 == "" {
				list[i].SHA256 = normalizeSHA256(asset.Digest)
			}
		}
		return list, nil
	}

	var list []Image
	for _, asset := range rel.Assets {
		img, ok := parseFirmwareName(asset.Name)
		if !ok {
			continue
		}
		img.URL = asset.BrowserDownloadURL
		img.Size = asset.Size
		img.SHA256 = normalizeSHA256(asset.Digest)
		list = append(list, img)
	}
	return list, nil
}

// Choose returns the newest release that contains a match, then the highest patch in that release.
func Choose(releases []Release, module, hardware string) (Release, Image, error) {
	if len(releases) == 0 {
		return Release{}, Image{}, fmt.Errorf("no releases")
	}
	var tried []Image
	for _, rel := range releases {
		img, err := Select(rel.Images, module, hardware)
		if err == nil {
			return rel, img, nil
		}
		tried = append(tried, rel.Images...)
	}
	return Release{}, Image{}, fmt.Errorf("no %s firmware for %s\n%s", module, hardware, formatImages(tried))
}

func (c *Client) Download(ctx context.Context, img Image, dest string) error {
	if img.URL == "" {
		return fmt.Errorf("no download URL for %s", img.File)
	}
	if img.SHA256 != "" {
		if sum, err := fileSHA256(dest); err == nil && stringsEqualFold(sum, img.SHA256) {
			return nil
		}
	}
	if err := os.MkdirAll(filepath.Dir(dest), 0o755); err != nil {
		return err
	}
	body, err := c.get(ctx, img.URL)
	if err != nil {
		return err
	}
	defer body.Close()

	partial := dest + ".partial"
	f, err := os.Create(partial)
	if err != nil {
		return err
	}
	sum, copyErr := copySHA256(f, body)
	closeErr := f.Close()
	if copyErr != nil {
		os.Remove(partial)
		return copyErr
	}
	if closeErr != nil {
		os.Remove(partial)
		return closeErr
	}
	if img.SHA256 != "" && !stringsEqualFold(sum, img.SHA256) {
		os.Remove(partial)
		return fmt.Errorf("%s sha256 %s, release says %s", img.File, sum, img.SHA256)
	}
	os.Remove(dest)
	if err := os.Rename(partial, dest); err != nil {
		os.Remove(partial)
		return err
	}
	return nil
}

func stringsEqualFold(a, b string) bool {
	return normalizeSHA256(a) == normalizeSHA256(b)
}

func (c *Client) getJSON(ctx context.Context, url string, dest any) error {
	body, err := c.get(ctx, url)
	if err != nil {
		return err
	}
	defer body.Close()
	if err := json.NewDecoder(body).Decode(dest); err != nil {
		return fmt.Errorf("%s: %w", url, err)
	}
	return nil
}

func (c *Client) get(ctx context.Context, url string) (io.ReadCloser, error) {
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, url, nil)
	if err != nil {
		return nil, err
	}
	req.Header.Set("User-Agent", "loco-flash")
	req.Header.Set("Accept", "application/vnd.github+json")
	if c.Token != "" {
		req.Header.Set("Authorization", "Bearer "+c.Token)
	}
	resp, err := c.http().Do(req)
	if err != nil {
		return nil, err
	}
	if resp.StatusCode != http.StatusOK {
		defer resp.Body.Close()
		msg, _ := io.ReadAll(io.LimitReader(resp.Body, 512))
		return nil, fmt.Errorf("GET %s: %s (%s)", url, resp.Status, strings.TrimSpace(string(msg)))
	}
	return resp.Body, nil
}

func cachePath(repo, tag, name string) (string, error) {
	root, err := os.UserCacheDir()
	if err != nil {
		root = os.TempDir()
	}
	repo, err = parseRepo(repo)
	if err != nil {
		return "", err
	}
	return filepath.Join(root, "loco-flash", filepath.FromSlash(repo), tag, name), nil
}
