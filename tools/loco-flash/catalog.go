package main

import (
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strconv"
	"strings"
)

// Image is one firmware file from a GitHub release.
// Hardware is major.minor. Version is major.minor.patch.
type Image struct {
	Module   string `json:"module"`
	Hardware string `json:"hardware"`
	Version  string `json:"version"`
	File     string `json:"file"`
	MCU      string `json:"mcu,omitempty"`
	Protocol string `json:"protocol,omitempty"`
	Baud     int    `json:"baud,omitempty"`
	SHA256   string `json:"sha256,omitempty"`
	URL      string `json:"-"`
	Size     int64  `json:"-"`
}

type manifestFile struct {
	Modules []Image `json:"modules"`
}

type parsedVersion struct {
	Major int
	Minor int
	Patch int
	Exact bool
}

func (v parsedVersion) hardware() string {
	return fmt.Sprintf("%d.%d", v.Major, v.Minor)
}

func (v parsedVersion) String() string {
	return fmt.Sprintf("%d.%d.%d", v.Major, v.Minor, v.Patch)
}

var firmwareName = regexp.MustCompile(`(?i)^([a-z0-9]+)_v([0-9]+)_([0-9]+)_([0-9]+)\.(hex|bin)$`)

func parseFirmwareName(name string) (Image, bool) {
	m := firmwareName.FindStringSubmatch(filepath.Base(name))
	if m == nil {
		return Image{}, false
	}
	major, _ := strconv.Atoi(m[2])
	minor, _ := strconv.Atoi(m[3])
	patch, _ := strconv.Atoi(m[4])
	ext := strings.ToLower(m[5])
	img := Image{
		Module:   strings.ToLower(m[1]),
		Hardware: fmt.Sprintf("%d.%d", major, minor),
		Version:  fmt.Sprintf("%d.%d.%d", major, minor, patch),
		File:     filepath.Base(name),
		MCU:      "atmega328p",
		Protocol: "arduino",
		Baud:     115200,
	}
	if ext == "bin" {
		img.MCU = "esp32s3"
		img.Protocol = "esptool"
		img.Baud = 0
	}
	return img, true
}

func parseVersion(s string) (parsedVersion, error) {
	s = strings.TrimSpace(strings.TrimPrefix(strings.ToLower(s), "v"))
	parts := strings.Split(s, ".")
	if len(parts) != 2 && len(parts) != 3 {
		return parsedVersion{}, fmt.Errorf("version %q must be major.minor or major.minor.patch", s)
	}
	nums := make([]int, len(parts))
	for i, part := range parts {
		n, err := strconv.Atoi(part)
		if err != nil || n < 0 {
			return parsedVersion{}, fmt.Errorf("version %q must be major.minor or major.minor.patch", s)
		}
		nums[i] = n
	}
	v := parsedVersion{Major: nums[0], Minor: nums[1], Exact: len(nums) == 3}
	if v.Exact {
		v.Patch = nums[2]
	}
	return v, nil
}

func (img Image) version() (parsedVersion, error) {
	v, err := parseVersion(img.Version)
	if err != nil {
		return parsedVersion{}, fmt.Errorf("%s: %w", img.File, err)
	}
	return v, nil
}

// Select picks the firmware for a module and hardware.
// hardware is "2.1" (highest patch) or "2.1.12" (that patch only).
func Select(images []Image, module, hardware string) (Image, error) {
	module = strings.ToLower(strings.TrimSpace(module))
	if module == "" {
		return Image{}, fmt.Errorf("module is required")
	}
	want, err := parseVersion(hardware)
	if err != nil {
		return Image{}, err
	}

	var match []Image
	for _, img := range images {
		if img.Module != module {
			continue
		}
		got, err := img.version()
		if err != nil {
			return Image{}, err
		}
		if got.Major != want.Major || got.Minor != want.Minor {
			continue
		}
		if want.Exact && got.Patch != want.Patch {
			continue
		}
		match = append(match, img)
	}
	if len(match) == 0 {
		return Image{}, fmt.Errorf("no %s firmware for %s\n%s", module, hardware, formatImages(images))
	}
	sort.Slice(match, func(i, j int) bool {
		vi, _ := match[i].version()
		vj, _ := match[j].version()
		return vi.Patch > vj.Patch
	})
	return match[0], nil
}

func (img Image) flashable() error {
	switch strings.ToLower(img.Protocol) {
	case "", "arduino":
	default:
		return fmt.Errorf("%s uses %s; loco-flash writes ATmega hex files over an FTDI adapter", img.File, img.Protocol)
	}
	if !strings.EqualFold(filepath.Ext(img.File), ".hex") {
		return fmt.Errorf("%s is not a .hex file; ESP32 images are not written over FTDI", img.File)
	}
	return nil
}

func formatImages(images []Image) string {
	if len(images) == 0 {
		return "available: none"
	}
	sorted := append([]Image(nil), images...)
	sort.Slice(sorted, func(i, j int) bool {
		if sorted[i].Module != sorted[j].Module {
			return sorted[i].Module < sorted[j].Module
		}
		return sorted[i].Version < sorted[j].Version
	})
	var b strings.Builder
	b.WriteString("available:")
	for _, img := range sorted {
		fmt.Fprintf(&b, "\n  %s  %s  %s", img.Module, img.Version, img.File)
	}
	return b.String()
}

func readManifest(r io.Reader) ([]Image, error) {
	var doc manifestFile
	dec := json.NewDecoder(r)
	dec.DisallowUnknownFields()
	if err := dec.Decode(&doc); err != nil {
		return nil, fmt.Errorf("manifest: %w", err)
	}
	for i := range doc.Modules {
		img := &doc.Modules[i]
		parsed, ok := parseFirmwareName(img.File)
		if !ok {
			return nil, fmt.Errorf("manifest file %q is not module_vMAJOR_MINOR_PATCH.hex", img.File)
		}
		if img.Module == "" {
			img.Module = parsed.Module
		}
		img.Module = strings.ToLower(img.Module)
		if img.Version == "" {
			img.Version = parsed.Version
		}
		if img.Hardware == "" {
			img.Hardware = parsed.Hardware
		}
		if img.MCU == "" {
			img.MCU = parsed.MCU
		}
		if img.Protocol == "" {
			img.Protocol = parsed.Protocol
		}
		if img.Baud == 0 {
			img.Baud = parsed.Baud
		}
		if img.Module != parsed.Module {
			return nil, fmt.Errorf("manifest %s belongs to %s, file name says %s", img.File, img.Module, parsed.Module)
		}
		got, err := img.version()
		if err != nil {
			return nil, err
		}
		if got.String() != parsed.Version || got.hardware() != parsed.Hardware {
			return nil, fmt.Errorf("manifest version for %s does not match its file name", img.File)
		}
		if img.Hardware != parsed.Hardware {
			return nil, fmt.Errorf("manifest hardware for %s does not match its file name", img.File)
		}
		img.SHA256 = normalizeSHA256(img.SHA256)
	}
	return doc.Modules, nil
}

func writeManifest(w io.Writer, images []Image) error {
	doc := manifestFile{Modules: images}
	enc := json.NewEncoder(w)
	enc.SetIndent("", "  ")
	if err := enc.Encode(doc); err != nil {
		return err
	}
	return nil
}

func manifestFromDir(dir string) ([]Image, error) {
	entries, err := os.ReadDir(dir)
	if err != nil {
		return nil, err
	}
	var images []Image
	for _, entry := range entries {
		if entry.IsDir() {
			continue
		}
		img, ok := parseFirmwareName(entry.Name())
		if !ok || img.Protocol != "arduino" {
			continue
		}
		sum, err := fileSHA256(filepath.Join(dir, entry.Name()))
		if err != nil {
			return nil, err
		}
		img.SHA256 = sum
		images = append(images, img)
	}
	sort.Slice(images, func(i, j int) bool {
		if images[i].Module != images[j].Module {
			return images[i].Module < images[j].Module
		}
		return images[i].File < images[j].File
	})
	if len(images) == 0 {
		return nil, fmt.Errorf("no module_vMAJOR_MINOR_PATCH.hex files in %s", dir)
	}
	return images, nil
}

func copySHA256(w io.Writer, r io.Reader) (string, error) {
	h := sha256.New()
	if _, err := io.Copy(io.MultiWriter(w, h), r); err != nil {
		return "", err
	}
	return hex.EncodeToString(h.Sum(nil)), nil
}

func fileSHA256(path string) (string, error) {
	f, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer f.Close()
	h := sha256.New()
	if _, err := io.Copy(h, f); err != nil {
		return "", err
	}
	return hex.EncodeToString(h.Sum(nil)), nil
}

func normalizeSHA256(s string) string {
	s = strings.TrimSpace(strings.ToLower(s))
	s = strings.TrimPrefix(s, "sha256:")
	return s
}
