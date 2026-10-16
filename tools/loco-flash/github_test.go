package main

import (
	"context"
	"fmt"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestReleaseWithoutManifest(t *testing.T) {
	const hex = ":020000020000FC\n"
	sum := shaOf(t, hex)
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.Header.Get("User-Agent") == "" {
			t.Error("missing user agent")
		}
		switch r.URL.Path {
		case "/repos/loco-can/loco-can-software/releases":
			w.Header().Set("Content-Type", "application/json")
			_, _ = w.Write([]byte(`[{
				"tag_name":"controller",
				"name":"Controller V2.1.0",
				"prerelease":true,
				"published_at":"2026-10-13T06:57:14Z",
				"assets":[{
					"name":"controller_v2_1_0.hex",
					"browser_download_url":"` + absURL(r, "/dl/controller_v2_1_0.hex") + `",
					"size":` + fmt.Sprintf("%d", len(hex)) + `,
					"digest":"sha256:` + sum + `"
				}]
			}]`))
		case "/dl/controller_v2_1_0.hex":
			_, _ = w.Write([]byte(hex))
		default:
			http.NotFound(w, r)
		}
	}))
	defer srv.Close()

	client := &Client{Repo: defaultRepo, BaseURL: srv.URL, HTTP: srv.Client()}
	releases, err := client.Releases(context.Background(), "", false)
	if err != nil {
		t.Fatal(err)
	}
	rel, img, err := Choose(releases, "controller", "2.1")
	if err != nil {
		t.Fatal(err)
	}
	if rel.Tag != "controller" || img.SHA256 != sum {
		t.Fatalf("release %+v image %+v", rel, img)
	}
	dest := filepath.Join(t.TempDir(), "controller_v2_1_0.hex")
	if err := client.Download(context.Background(), img, dest); err != nil {
		t.Fatal(err)
	}
	if err := client.Download(context.Background(), img, dest); err != nil {
		t.Fatal(err)
	}
	got, err := os.ReadFile(dest)
	if err != nil || string(got) != hex {
		t.Fatalf("file %q err %v", got, err)
	}
}

func TestStableSkipsPrerelease(t *testing.T) {
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		_, _ = w.Write([]byte(`[{
			"tag_name":"controller",
			"prerelease":true,
			"published_at":"2026-10-13T06:57:14Z",
			"assets":[{"name":"controller_v2_1_0.hex","browser_download_url":"http://example/x","digest":""}]
		}]`))
	}))
	defer srv.Close()
	client := &Client{Repo: defaultRepo, BaseURL: srv.URL, HTTP: srv.Client()}
	if _, err := client.Releases(context.Background(), "", true); err == nil {
		t.Fatal("expected no stable release")
	}
}

func absURL(r *http.Request, path string) string {
	return "http://" + r.Host + path
}

func shaOf(t *testing.T, body string) string {
	t.Helper()
	sum, err := copySHA256(&strings.Builder{}, strings.NewReader(body))
	if err != nil {
		t.Fatal(err)
	}
	return sum
}
