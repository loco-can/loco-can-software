package main

import (
	"bufio"
	"bytes"
	"context"
	"errors"
	"net"
	"strings"
	"testing"
	"time"
)

func TestOptibootProgramsAndVerifies(t *testing.T) {
	mem := make([]byte, 200)
	for i := range mem {
		mem[i] = byte(i + 1)
	}
	img, err := parseIntelHex(bytesReader(encodeHex(mem)))
	if err != nil {
		t.Fatal(err)
	}
	host, boot := net.Pipe()
	defer host.Close()
	go runFakeBoot(t, boot, 0x0f, false)
	port := &pipePort{c: host}
	err = fastOptiboot().program(context.Background(), port, img, nil)
	if err != nil {
		t.Fatal(err)
	}
	if len(port.dtr) < 3 || port.dtr[0] || !port.dtr[1] || port.dtr[2] {
		t.Fatalf("DTR pulse %v", port.dtr)
	}
}

func TestOptibootRejectsForeignSignature(t *testing.T) {
	img, err := parseIntelHex(bytesReader(encodeHex([]byte{0x11})))
	if err != nil {
		t.Fatal(err)
	}
	host, boot := net.Pipe()
	defer host.Close()
	go runFakeBoot(t, boot, 0x01, false)
	err = fastOptiboot().program(context.Background(), &pipePort{c: host}, img, nil)
	if err == nil || errors.Is(err, errNoContact) {
		t.Fatal(err)
	}
}

func TestOptibootVerifyFailure(t *testing.T) {
	img, err := parseIntelHex(bytesReader(encodeHex([]byte{0x11, 0x22})))
	if err != nil {
		t.Fatal(err)
	}
	host, boot := net.Pipe()
	defer host.Close()
	go runFakeBoot(t, boot, 0x0f, true)
	err = fastOptiboot().program(context.Background(), &pipePort{c: host}, img, nil)
	if err == nil || !strings.Contains(err.Error(), "Prüfung") {
		t.Fatal(err)
	}
}

func TestNoContactDoesNotLookLikeAWrite(t *testing.T) {
	img, err := parseIntelHex(bytesReader(encodeHex([]byte{0x11})))
	if err != nil {
		t.Fatal(err)
	}
	port := &timeoutPort{}
	ob := fastOptiboot()
	ob.syncFor = 20 * time.Millisecond
	ob.readWait = time.Millisecond
	err = ob.program(context.Background(), port, img, nil)
	if !errors.Is(err, errNoContact) {
		t.Fatal(err)
	}
}

func fastOptiboot() optiboot {
	return optiboot{
		resets:   1,
		readWait: 200 * time.Millisecond,
		cmdWait:  time.Second,
		syncFor:  time.Second,
	}
}

type pipePort struct {
	c   net.Conn
	dtr []bool
}

func (p *pipePort) Read(b []byte) (int, error)  { return p.c.Read(b) }
func (p *pipePort) Write(b []byte) (int, error) { return p.c.Write(b) }
func (p *pipePort) SetDTR(v bool) error {
	p.dtr = append(p.dtr, v)
	return nil
}
func (p *pipePort) SetRTS(bool) error { return nil }
func (p *pipePort) SetReadTimeout(d time.Duration) error {
	if d <= 0 {
		return p.c.SetReadDeadline(time.Time{})
	}
	return p.c.SetReadDeadline(time.Now().Add(d))
}
func (p *pipePort) ResetInputBuffer() error {
	_ = p.c.SetReadDeadline(time.Now())
	buf := make([]byte, 64)
	for {
		_, err := p.c.Read(buf)
		if err != nil {
			break
		}
	}
	return p.c.SetReadDeadline(time.Time{})
}

type timeoutPort struct{}

func (timeoutPort) Read([]byte) (int, error)           { return 0, nil }
func (timeoutPort) Write(p []byte) (int, error)        { return len(p), nil }
func (timeoutPort) SetDTR(bool) error                  { return nil }
func (timeoutPort) SetRTS(bool) error                  { return nil }
func (timeoutPort) SetReadTimeout(time.Duration) error { return nil }
func (timeoutPort) ResetInputBuffer() error            { return nil }

func bytesReader(b []byte) *bytes.Reader { return bytes.NewReader(b) }

func runFakeBoot(t *testing.T, conn net.Conn, sig2 byte, corrupt bool) {
	t.Helper()
	defer conn.Close()
	_ = conn.SetDeadline(time.Now().Add(3 * time.Second))
	r := bufio.NewReader(conn)
	flash := make([]byte, flashBytes)
	for i := range flash {
		flash[i] = 0xFF
	}
	var addr uint32
	get := func() byte {
		b, err := r.ReadByte()
		if err != nil {
			return 0
		}
		return b
	}
	discard := func(n int) {
		for i := 0; i < n; i++ {
			get()
		}
	}
	reply := func(payload []byte) {
		if get() != stkCRC {
			t.Errorf("missing CRC")
		}
		_, _ = conn.Write([]byte{stkInsync})
		if len(payload) > 0 {
			_, _ = conn.Write(payload)
		}
		_, _ = conn.Write([]byte{stkOK})
	}
	for {
		cmd, err := r.ReadByte()
		if err != nil {
			return
		}
		switch cmd {
		case stkSetDevice:
			discard(20)
			reply(nil)
		case stkSetDeviceExt:
			discard(5)
			reply(nil)
		case stkLoadAddr:
			lo, hi := get(), get()
			addr = (uint32(lo) | uint32(hi)<<8) * 2
			reply(nil)
		case stkProgPage:
			n := int(get())<<8 | int(get())
			_ = get()
			buf := make([]byte, n)
			for i := 0; i < n; i++ {
				buf[i] = get()
			}
			if corrupt && n > 0 {
				buf[0] ^= 0xFF
			}
			if int(addr)+n <= len(flash) {
				copy(flash[addr:], buf)
			}
			reply(nil)
		case stkReadPage:
			n := int(get())<<8 | int(get())
			_ = get()
			reply(flash[addr : addr+uint32(n)])
		case stkReadSign:
			reply([]byte{0x1e, 0x95, sig2})
		default:
			reply(nil)
		}
	}
}
