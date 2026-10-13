package main

import (
	"bufio"
	"encoding/hex"
	"fmt"
	"io"
	"strings"
)

const (
	pageSize   = 128
	flashBytes = 32 * 1024
)

// hexImage is the ATmega328P application image from an Intel HEX file.
// Missing bytes inside a used page are filled with 0xFF when programming,
// which matches avrdude -D (no chip erase, page erase on write).
type hexImage struct {
	bytes map[uint32]byte
}

type flashPage struct {
	addr uint32
	data []byte
}

func parseIntelHex(r io.Reader) (*hexImage, error) {
	sc := bufio.NewScanner(r)
	img := &hexImage{bytes: map[uint32]byte{}}
	var base uint32
	eof := false
	lineNo := 0
	for sc.Scan() {
		lineNo++
		line := strings.TrimSpace(sc.Text())
		if line == "" {
			continue
		}
		if eof {
			return nil, fmt.Errorf("line %d: data after end of file", lineNo)
		}
		if !strings.HasPrefix(line, ":") {
			return nil, fmt.Errorf("line %d: not an Intel HEX record", lineNo)
		}
		raw, err := hex.DecodeString(line[1:])
		if err != nil {
			return nil, fmt.Errorf("line %d: %w", lineNo, err)
		}
		if len(raw) < 5 {
			return nil, fmt.Errorf("line %d: short record", lineNo)
		}
		count := int(raw[0])
		if len(raw) != 5+count {
			return nil, fmt.Errorf("line %d: length is %d, record has %d bytes", lineNo, count, len(raw)-5)
		}
		var sum byte
		for _, b := range raw {
			sum += b
		}
		if sum != 0 {
			return nil, fmt.Errorf("line %d: bad checksum", lineNo)
		}
		addr := uint32(raw[1])<<8 | uint32(raw[2])
		kind := raw[3]
		data := raw[4 : 4+count]
		switch kind {
		case 0x00:
			for i, b := range data {
				abs := base + addr + uint32(i)
				if abs >= flashBytes {
					return nil, fmt.Errorf("line %d: address %d is outside ATmega328P flash", lineNo, abs)
				}
				img.bytes[abs] = b
			}
		case 0x01:
			eof = true
		case 0x02:
			if count != 2 {
				return nil, fmt.Errorf("line %d: extended segment address must be 2 bytes", lineNo)
			}
			seg := uint32(data[0])<<8 | uint32(data[1])
			base = seg << 4
		case 0x03, 0x05:
			// Start address. The bootloader does not use it.
		case 0x04:
			if count != 2 {
				return nil, fmt.Errorf("line %d: extended linear address must be 2 bytes", lineNo)
			}
			hi := uint32(data[0])<<8 | uint32(data[1])
			base = hi << 16
		default:
			return nil, fmt.Errorf("line %d: unsupported record type %02X", lineNo, kind)
		}
	}
	if err := sc.Err(); err != nil {
		return nil, err
	}
	if !eof {
		return nil, fmt.Errorf("intel hex has no end record")
	}
	return img, nil
}

func (m *hexImage) pages() []flashPage {
	if len(m.bytes) == 0 {
		return nil
	}
	var max uint32
	for addr := range m.bytes {
		if addr > max {
			max = addr
		}
	}
	var out []flashPage
	for base := uint32(0); base <= max; base += pageSize {
		page := flashPage{addr: base, data: make([]byte, pageSize)}
		for i := range page.data {
			page.data[i] = 0xFF
		}
		used := false
		for i := uint32(0); i < pageSize; i++ {
			if b, ok := m.bytes[base+i]; ok {
				page.data[i] = b
				used = true
			}
		}
		if used {
			out = append(out, page)
		}
	}
	return out
}
