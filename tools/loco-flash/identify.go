package main

import (
	"bytes"
	"context"
	"fmt"
	"regexp"
	"strconv"
	"strings"
	"time"
)

// ModuleInfo is the type and firmware version running on a connected board.
type ModuleInfo struct {
	Module  string `json:"module"`
	Version string `json:"version,omitempty"`
	TypeID  byte   `json:"type_id,omitempty"`
	CAN     *bool  `json:"can,omitempty"`
	Raw     string `json:"raw,omitempty"`
}

var (
	locoIdentifyLine = regexp.MustCompile(`(?i)LOCO\s+type=([a-z][a-z0-9]*)(?:\s+ver=([0-9]+\.[0-9]+(?:\.[0-9]+)?))?(?:\s+id=0x([0-9a-f]+))?(?:\s+can=([01]))?`)
	legacyModuleLine = regexp.MustCompile(`(?i)MODULE:\s*(?:.*[/\\])?module[/\\]([a-z][a-z0-9]*)[/\\]`)
)

func parseIdentifyText(s string) (ModuleInfo, bool) {
	var fallback ModuleInfo
	var haveFallback bool
	for _, line := range strings.Split(s, "\n") {
		info, ok := parseIdentifyLine(strings.TrimSpace(line))
		if !ok {
			continue
		}
		if strings.HasPrefix(strings.ToUpper(info.Raw), "LOCO ") {
			return info, true
		}
		if !haveFallback {
			fallback = info
			haveFallback = true
		}
	}
	return fallback, haveFallback
}

func parseIdentifyLine(line string) (ModuleInfo, bool) {
	line = strings.TrimSpace(line)
	if m := locoIdentifyLine.FindStringSubmatch(line); m != nil {
		info := ModuleInfo{
			Module:  strings.ToLower(m[1]),
			Version: m[2],
			Raw:     line,
		}
		if m[3] != "" {
			n, err := strconv.ParseUint(m[3], 16, 8)
			if err == nil {
				info.TypeID = byte(n)
			}
		}
		if m[4] != "" {
			on := m[4] == "1"
			info.CAN = &on
		}
		return info, true
	}
	if m := legacyModuleLine.FindStringSubmatch(line); m != nil {
		return ModuleInfo{Module: strings.ToLower(m[1]), Raw: line}, true
	}
	return ModuleInfo{}, false
}

func probeModule(ctx context.Context, portName string) (ModuleInfo, error) {
	if err := ctx.Err(); err != nil {
		return ModuleInfo{}, err
	}
	port, err := openSerial(portName, 115200)
	if err != nil {
		return ModuleInfo{}, err
	}
	defer port.Close()
	_ = setLines(port, false)

	// Opening an FTDI port already pulses RESET. Listen for the boot
	// banner; do not drain it and do not send '?' while Optiboot is up.
	if info, ok := readIdentify(ctx, port, 2*time.Second); ok {
		return info, nil
	}
	if info, ok := askIdentify(ctx, port, 400*time.Millisecond); ok {
		return info, nil
	}

	if err := resetSketch(port); err != nil {
		return ModuleInfo{}, err
	}
	info, ok := readIdentify(ctx, port, 2500*time.Millisecond)
	if ok {
		return info, nil
	}
	if asked, ok := askIdentify(ctx, port, 800*time.Millisecond); ok {
		return asked, nil
	}
	if info.Raw != "" {
		return ModuleInfo{}, fmt.Errorf("%s: kein Modul auf der seriellen Schnittstelle (%s)", portName, info.Raw)
	}
	return ModuleInfo{}, fmt.Errorf("%s: kein Modul auf der seriellen Schnittstelle", portName)
}

func resetSketch(w wire) error {
	if err := setLines(w, false); err != nil {
		return err
	}
	sleep(50 * time.Millisecond)
	if err := setLines(w, true); err != nil {
		return err
	}
	sleep(50 * time.Millisecond)
	if err := setLines(w, false); err != nil {
		return err
	}
	return nil
}

func askIdentify(ctx context.Context, w wire, wait time.Duration) (ModuleInfo, bool) {
	if err := writeRaw(w, []byte("?")); err != nil {
		return ModuleInfo{}, false
	}
	return readIdentify(ctx, w, wait)
}

func writeRaw(w wire, p []byte) error {
	for len(p) > 0 {
		n, err := w.Write(p)
		if n > 0 {
			p = p[n:]
		}
		if err != nil {
			return err
		}
		if n == 0 {
			return fmt.Errorf("Schreiben ohne Fortschritt")
		}
	}
	return nil
}

func readIdentify(ctx context.Context, w wire, wait time.Duration) (ModuleInfo, bool) {
	var buf bytes.Buffer
	deadline := time.Now().Add(wait)
	tmp := make([]byte, 64)
	for time.Now().Before(deadline) {
		if err := ctx.Err(); err != nil {
			return ModuleInfo{}, false
		}
		remain := time.Until(deadline)
		if remain <= 0 {
			break
		}
		if remain > 50*time.Millisecond {
			remain = 50 * time.Millisecond
		}
		if err := w.SetReadTimeout(remain); err != nil {
			return ModuleInfo{}, false
		}
		n, err := w.Read(tmp)
		if n > 0 {
			buf.Write(tmp[:n])
			if info, ok := parseIdentifyText(buf.String()); ok {
				return info, true
			}
		}
		if err != nil && !isTimeout(err) {
			return ModuleInfo{}, false
		}
	}
	info, ok := parseIdentifyText(buf.String())
	if !ok && buf.Len() > 0 {
		return ModuleInfo{Raw: strings.TrimSpace(printableTail(buf.String(), 180))}, false
	}
	return info, ok
}

func printableTail(s string, n int) string {
	var b strings.Builder
	for _, r := range s {
		if r == '\n' || r == '\r' || r == '\t' || (r >= 32 && r < 127) {
			b.WriteRune(r)
		} else {
			b.WriteByte('.')
		}
	}
	out := b.String()
	if len(out) > n {
		return out[len(out)-n:]
	}
	return out
}
