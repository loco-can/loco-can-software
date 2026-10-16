package main

import "testing"

func TestParseIdentifyLine(t *testing.T) {
	info, ok := parseIdentifyLine("LOCO type=electric ver=2.1.0 id=0x20 can=1")
	if !ok || info.Module != "electric" || info.Version != "2.1.0" || info.TypeID != 0x20 {
		t.Fatalf("%+v ok=%v", info, ok)
	}
	if info.CAN == nil || !*info.CAN {
		t.Fatal("can")
	}
	legacy, ok := parseIdentifyLine("MODULE: src/module/switch/config.h")
	if !ok || legacy.Module != "switch" || legacy.Version != "" {
		t.Fatalf("legacy %+v", legacy)
	}
	if _, ok := parseIdentifyLine("***********************"); ok {
		t.Fatal("banner")
	}
}

func TestParseIdentifyTextSkipsBanner(t *testing.T) {
	text := "* Welcome to Loco-Can *\nMODULE: src/module/electric/config.h\nLOCO type=electric ver=2.1.0 id=0x20 can=0\n"
	info, ok := parseIdentifyText(text)
	if !ok || info.Module != "electric" || info.Version != "2.1.0" {
		t.Fatalf("%+v", info)
	}
}

func TestMatchProbedImageSameOrNewer(t *testing.T) {
	images := []Image{
		{Module: "electric", Version: "2.1.0", File: "electric_v2_1_0.hex", Protocol: "arduino"},
		{Module: "electric", Version: "2.1.12", File: "electric_v2_1_12.hex", Protocol: "arduino"},
		{Module: "switch", Version: "2.1.0", File: "switch_v2_1_0.hex", Protocol: "arduino"},
	}
	got, ok := matchProbedImage(images, ModuleInfo{Module: "electric", Version: "2.1.0"})
	if !ok || got.File != "electric_v2_1_12.hex" {
		t.Fatalf("same or newer %+v", got)
	}
	latest, ok := matchProbedImage(images, ModuleInfo{Module: "electric", Version: "2.1"})
	if !ok || latest.File != "electric_v2_1_12.hex" {
		t.Fatalf("hardware %+v", latest)
	}
	if _, ok := matchProbedImage(images, ModuleInfo{Module: "electric", Version: "3.0.0"}); ok {
		t.Fatal("must not pick older than the board")
	}
}

func TestMatchProbedReleaseSameOrNewer(t *testing.T) {
	releases := []Release{
		{Tag: "v2", Images: []Image{mustImage("electric_v2_1_12.hex"), mustImage("switch_v2_1_0.hex")}},
		{Tag: "v1", Images: []Image{mustImage("electric_v2_1_0.hex")}},
		{Tag: "v0", Images: []Image{mustImage("electric_v2_0_0.hex")}},
	}
	idx, img, ok := matchProbedRelease(releases, ModuleInfo{Module: "electric", Version: "2.1.0"})
	if !ok || idx != 0 || img.File != "electric_v2_1_12.hex" {
		t.Fatalf("newer %+v idx=%d ok=%v", img, idx, ok)
	}
	idx, img, ok = matchProbedRelease(releases, ModuleInfo{Module: "electric", Version: "2.1.12"})
	if !ok || idx != 0 || img.File != "electric_v2_1_12.hex" {
		t.Fatalf("same %+v idx=%d", img, idx)
	}
	if _, _, ok := matchProbedRelease(releases, ModuleInfo{Module: "electric", Version: "3.0.0"}); ok {
		t.Fatal("must not pick older than the board")
	}
	idx, img, ok = matchProbedRelease(releases, ModuleInfo{Module: "switch"})
	if !ok || img.File != "switch_v2_1_0.hex" || idx != 0 {
		t.Fatalf("no version %+v idx=%d", img, idx)
	}
}
