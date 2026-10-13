package main

import (
	"bytes"
	"fmt"
	"os"
	"strings"
	"testing"
)

func TestIntelHexPagesAndHoles(t *testing.T) {
	mem := make([]byte, 200)
	for i := range mem {
		mem[i] = byte(i)
	}
	img, err := parseIntelHex(bytes.NewReader(encodeHex(mem)))
	if err != nil {
		t.Fatal(err)
	}
	pages := img.pages()
	if len(pages) != 2 {
		t.Fatalf("pages %d", len(pages))
	}
	if pages[0].addr != 0 || pages[1].addr != 128 {
		t.Fatalf("addrs %x %x", pages[0].addr, pages[1].addr)
	}
	if pages[1].data[199-128] != 199 || pages[1].data[127] != 0xFF {
		t.Fatalf("page fill %+v", pages[1].data[70:])
	}
}

func TestIntelHexExtendedAddress(t *testing.T) {
	// Base 0 plus one data byte at address 0.
	text := ":020000040000FA\n:01000000649B\n:00000001FF\n"
	img, err := parseIntelHex(strings.NewReader(text))
	if err != nil {
		t.Fatal(err)
	}
	if img.bytes[0] != 0x64 {
		t.Fatalf("byte %02x", img.bytes[0])
	}
}

func TestIntelHexRejectsBadChecksumAndBootloader(t *testing.T) {
	if _, err := parseIntelHex(strings.NewReader(":0100000000FE\n:00000001FF\n")); err == nil {
		t.Fatal("checksum should fail")
	}
	// Extended base 0x10000 plus offset 0x8000 is past the 32 KiB flash.
	text := ":020000040001F9\n:01800000007F\n:00000001FF\n"
	if _, err := parseIntelHex(strings.NewReader(text)); err == nil {
		t.Fatal("address past flash should fail")
	}
}

func TestOptionalFirmwareHex(t *testing.T) {
	path := os.Getenv("LOCO_HEX")
	if path == "" {
		t.Skip()
	}
	f, err := os.Open(path)
	if err != nil {
		t.Fatal(err)
	}
	defer f.Close()
	img, err := parseIntelHex(f)
	if err != nil {
		t.Fatal(err)
	}
	pages := img.pages()
	if len(pages) == 0 {
		t.Fatal("no pages")
	}
	t.Logf("%d pages, last address 0x%04x", len(pages), pages[len(pages)-1].addr)
}

func encodeHex(mem []byte) []byte {
	var b strings.Builder
	b.WriteString(":020000040000FA\n")
	for off := 0; off < len(mem); off += 16 {
		n := 16
		if off+n > len(mem) {
			n = len(mem) - off
		}
		rec := []byte{byte(n), byte(off >> 8), byte(off), 0x00}
		rec = append(rec, mem[off:off+n]...)
		var sum byte
		for _, x := range rec {
			sum += x
		}
		fmt.Fprintf(&b, ":%02X%04X00", n, off)
		for _, x := range mem[off : off+n] {
			fmt.Fprintf(&b, "%02X", x)
		}
		fmt.Fprintf(&b, "%02X\n", byte(-sum))
	}
	b.WriteString(":00000001FF\n")
	return []byte(b.String())
}
