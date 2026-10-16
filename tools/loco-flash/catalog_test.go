package main

import (
	"bytes"
	"strings"
	"testing"
	"time"
)

func TestParseFirmwareName(t *testing.T) {
	img, ok := parseFirmwareName("controller_v2_1_12.hex")
	if !ok {
		t.Fatal("expected a match")
	}
	if img.Module != "controller" || img.Hardware != "2.1" || img.Version != "2.1.12" {
		t.Fatalf("parsed %+v", img)
	}
	if img.Protocol != "arduino" || img.Baud != 115200 || img.MCU != "atmega328p" {
		t.Fatalf("defaults %+v", img)
	}
	if _, ok := parseFirmwareName("firmware.hex"); ok {
		t.Fatal("generic firmware.hex must not match")
	}
	bin, ok := parseFirmwareName("wifi_v3_0_0.bin")
	if !ok || bin.Protocol != "esptool" {
		t.Fatalf("bin image %+v ok=%v", bin, ok)
	}
	if err := bin.flashable(); err == nil {
		t.Fatal("ESP32 image must not be offered for FTDI")
	}
}

func TestSelectHighestPatch(t *testing.T) {
	images := []Image{
		mustImage("controller_v2_0_0.hex"),
		mustImage("controller_v2_1_0.hex"),
		mustImage("controller_v2_1_12.hex"),
		mustImage("switch_v2_1_0.hex"),
	}
	got, err := Select(images, "controller", "2.1")
	if err != nil {
		t.Fatal(err)
	}
	if got.File != "controller_v2_1_12.hex" {
		t.Fatalf("got %s", got.File)
	}
	exact, err := Select(images, "Controller", "2.1.0")
	if err != nil {
		t.Fatal(err)
	}
	if exact.File != "controller_v2_1_0.hex" {
		t.Fatalf("got %s", exact.File)
	}
	if _, err := Select(images, "sensor", "2.1"); err == nil {
		t.Fatal("missing module should fail")
	}
}

func TestChooseNewestRelease(t *testing.T) {
	older := Release{
		Tag:         "controller",
		PublishedAt: time.Date(2026, 10, 13, 0, 0, 0, 0, time.UTC),
		Images:      []Image{mustImage("controller_v2_1_0.hex")},
	}
	newer := Release{
		Tag:         "switch",
		PublishedAt: time.Date(2026, 10, 14, 0, 0, 0, 0, time.UTC),
		Images:      []Image{mustImage("switch_v2_1_3.hex")},
	}
	rel, img, err := Choose([]Release{newer, older}, "controller", "2.1")
	if err != nil {
		t.Fatal(err)
	}
	if rel.Tag != "controller" || img.Version != "2.1.0" {
		t.Fatalf("got %s %s", rel.Tag, img.Version)
	}
}

func TestManifestRoundTrip(t *testing.T) {
	src := mustImage("electric_v2_1_12.hex")
	src.SHA256 = "abc"
	var buf bytes.Buffer
	if err := writeManifest(&buf, []Image{src}); err != nil {
		t.Fatal(err)
	}
	got, err := readManifest(&buf)
	if err != nil {
		t.Fatal(err)
	}
	if len(got) != 1 || got[0].File != src.File || got[0].Version != "2.1.12" || got[0].SHA256 != "abc" {
		t.Fatalf("got %+v", got)
	}
}

func TestManifestRejectsMismatch(t *testing.T) {
	body := `{"modules":[{"module":"switch","hardware":"2.1","version":"2.1.0","file":"controller_v2_1_0.hex"}]}`
	if _, err := readManifest(strings.NewReader(body)); err == nil {
		t.Fatal("expected mismatch")
	}
}

func TestParseRepo(t *testing.T) {
	for _, in := range []string{
		"loco-can/loco-can-software",
		"https://github.com/loco-can/loco-can-software",
		"https://github.com/loco-can/loco-can-software.git",
	} {
		got, err := parseRepo(in)
		if err != nil || got != "loco-can/loco-can-software" {
			t.Fatalf("%s -> %s %v", in, got, err)
		}
	}
}

func TestNormalizeDigest(t *testing.T) {
	if normalizeSHA256("sha256:ABC") != "abc" {
		t.Fatal(normalizeSHA256("sha256:ABC"))
	}
}

func mustImage(name string) Image {
	img, ok := parseFirmwareName(name)
	if !ok {
		panic(name)
	}
	return img
}
