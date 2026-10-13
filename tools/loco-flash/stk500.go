package main

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"os"
	"time"
)

const (
	stkOK           = 0x10
	stkInsync       = 0x14
	stkCRC          = 0x20
	stkGetSync      = 0x30
	stkSetDevice    = 0x42
	stkSetDeviceExt = 0x45
	stkEnter        = 0x50
	stkLeave        = 0x51
	stkLoadAddr     = 0x55
	stkProgPage     = 0x64
	stkReadPage     = 0x74
	stkReadSign     = 0x75
)

// Parameters avrdude sends for an ATmega328P with -c arduino.
// Optiboot ignores them. The old Nano bootloader stores them.
var m328pDevice = []byte{
	0x86, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x80, 0x04, 0x00,
	0x00, 0x00, 0x80, 0x00,
}

var m328pDeviceExt = []byte{0x05, 0x04, 0xd7, 0xc2, 0x00}

var (
	errNoContact = errors.New("Bootloader antwortet nicht")
	errTimeout   = errors.New("Zeitüberschreitung beim Lesen")
)

// wire is the piece of a serial port the programmer uses.
// SetDTR(true) asserts TIOCM_DTR, which drives an FTDI DTR pin low.
type wire interface {
	Read([]byte) (int, error)
	Write([]byte) (int, error)
	SetDTR(bool) error
	SetRTS(bool) error
	SetReadTimeout(time.Duration) error
	ResetInputBuffer() error
}

type flashLogger func(msg string, done, total int)

type optiboot struct {
	discharge time.Duration
	pulse     time.Duration
	bootWait  time.Duration
	syncFor   time.Duration
	readWait  time.Duration
	cmdWait   time.Duration
	resets    int
}

func standardOptiboot() optiboot {
	return optiboot{
		discharge: 250 * time.Millisecond,
		pulse:     100 * time.Microsecond,
		bootWait:  100 * time.Millisecond,
		syncFor:   500 * time.Millisecond,
		readWait:  50 * time.Millisecond,
		cmdWait:   2 * time.Second,
		resets:    3,
	}
}

// programHex writes an Intel HEX file to one ATmega328P. bauds is tried in
// order. 115200 is Optiboot. 57600 is the old Nano bootloader. A later baud
// is used only when the bootloader does not answer.
func programHex(ctx context.Context, portName string, data []byte, bauds []int, log flashLogger) error {
	if log == nil {
		log = func(string, int, int) {}
	}
	if len(bauds) == 0 {
		return fmt.Errorf("keine Baudrate")
	}
	mem, err := parseIntelHex(bytes.NewReader(data))
	if err != nil {
		return err
	}
	if len(mem.pages()) == 0 {
		return fmt.Errorf("HEX-Datei enthält keine Daten")
	}
	var last error
	for i, baud := range bauds {
		if err := ctx.Err(); err != nil {
			return err
		}
		if i > 0 {
			log(fmt.Sprintf("kein Kontakt bei %d, versuche %d (alter Nano-Bootloader)", bauds[i-1], baud), 0, 0)
		}
		err := programAtBaud(ctx, portName, baud, mem, log)
		if err == nil {
			return nil
		}
		if !errors.Is(err, errNoContact) {
			return err
		}
		last = err
	}
	return fmt.Errorf("%s: %w", portName, last)
}

func programAtBaud(ctx context.Context, portName string, baud int, mem *hexImage, log flashLogger) error {
	port, err := openSerial(portName, baud)
	if err != nil {
		return err
	}
	defer port.Close()
	return standardOptiboot().program(ctx, port, mem, log)
}

func (o optiboot) program(ctx context.Context, w wire, mem *hexImage, log flashLogger) error {
	if log == nil {
		log = func(string, int, int) {}
	}
	pages := mem.pages()
	if len(pages) == 0 {
		return fmt.Errorf("HEX-Datei enthält keine Daten")
	}
	if err := o.contact(ctx, w); err != nil {
		return err
	}
	if _, err := o.command(w, stkSetDevice, m328pDevice, 0); err != nil {
		return fmt.Errorf("SET_DEVICE: %w", err)
	}
	if _, err := o.command(w, stkSetDeviceExt, m328pDeviceExt, 0); err != nil {
		return fmt.Errorf("SET_DEVICE_EXT: %w", err)
	}
	if _, err := o.command(w, stkEnter, nil, 0); err != nil {
		return fmt.Errorf("ENTER_PROGMODE: %w", err)
	}
	sig, err := o.command(w, stkReadSign, nil, 3)
	if err != nil {
		return fmt.Errorf("READ_SIGN: %w", err)
	}
	if !is328P(sig) {
		_, _ = o.command(w, stkLeave, nil, 0)
		return fmt.Errorf("Signatur %02x %02x %02x passt nicht zu ATmega328P (1e 95 0f)", sig[0], sig[1], sig[2])
	}
	log(fmt.Sprintf("Kontakt, Signatur %02x %02x %02x", sig[0], sig[1], sig[2]), 0, len(pages))

	for i, page := range pages {
		if err := ctx.Err(); err != nil {
			return err
		}
		if err := o.writePage(w, page); err != nil {
			return fmt.Errorf("Seite 0x%04x: %w", page.addr, err)
		}
		msg := ""
		if i == 0 || i+1 == len(pages) || (i+1)%16 == 0 {
			msg = fmt.Sprintf("Seite %d von %d", i+1, len(pages))
		}
		log(msg, i+1, len(pages))
	}
	log("Prüfen", len(pages), len(pages))
	for _, page := range pages {
		if err := ctx.Err(); err != nil {
			return err
		}
		if err := o.readPage(w, page); err != nil {
			_, _ = o.command(w, stkLeave, nil, 0)
			return err
		}
	}
	if _, err := o.command(w, stkLeave, nil, 0); err != nil {
		return fmt.Errorf("LEAVE_PROGMODE: %w", err)
	}
	log("Fertig", len(pages), len(pages))
	return nil
}

func is328P(sig []byte) bool {
	if len(sig) != 3 || sig[0] != 0x1e || sig[1] != 0x95 {
		return false
	}
	// 0x0f is the ATmega328P. 0x14 is the ATmega328, same flash layout.
	return sig[2] == 0x0f || sig[2] == 0x14
}

func (o optiboot) contact(ctx context.Context, w wire) error {
	tries := o.resets
	if tries < 1 {
		tries = 1
	}
	for attempt := 0; attempt < tries; attempt++ {
		if err := ctx.Err(); err != nil {
			return err
		}
		if err := o.reset(w); err != nil {
			return err
		}
		if err := o.sync(ctx, w); err == nil {
			return nil
		}
	}
	return errNoContact
}

// reset follows avrdude's arduino programmer. On a negative-logic FTDI
// adapter, asserting DTR/RTS pulls the pin low and the falling edge through
// the 100 nF capacitor resets the ATmega. The line is released again so a
// board that wires DTR straight to RESET does not stay in reset.
func (o optiboot) reset(w wire) error {
	if err := setLines(w, false); err != nil {
		return err
	}
	sleep(o.discharge)
	if err := setLines(w, true); err != nil {
		return err
	}
	sleep(o.pulse)
	if err := setLines(w, false); err != nil {
		return err
	}
	sleep(o.bootWait)
	return nil
}

func setLines(w wire, asserted bool) error {
	if err := w.SetDTR(asserted); err != nil {
		return err
	}
	return w.SetRTS(asserted)
}

func (o optiboot) sync(ctx context.Context, w wire) error {
	window := o.syncFor
	if window <= 0 {
		window = 50 * time.Millisecond
	}
	deadline := time.Now().Add(window)
	for time.Now().Before(deadline) {
		if err := ctx.Err(); err != nil {
			return err
		}
		_ = w.ResetInputBuffer()
		if err := writeAll(w, []byte{stkGetSync, stkCRC}); err != nil {
			return err
		}
		buf, err := readExact(w, 2, o.readWait)
		if err == nil && len(buf) == 2 && buf[0] == stkInsync && buf[1] == stkOK {
			return nil
		}
	}
	return errNoContact
}

func (o optiboot) writePage(w wire, page flashPage) error {
	word := uint16(page.addr / 2)
	if _, err := o.command(w, stkLoadAddr, []byte{byte(word), byte(word >> 8)}, 0); err != nil {
		return err
	}
	args := []byte{byte(len(page.data) >> 8), byte(len(page.data)), 'F'}
	args = append(args, page.data...)
	_, err := o.command(w, stkProgPage, args, 0)
	return err
}

func (o optiboot) readPage(w wire, page flashPage) error {
	word := uint16(page.addr / 2)
	if _, err := o.command(w, stkLoadAddr, []byte{byte(word), byte(word >> 8)}, 0); err != nil {
		return err
	}
	got, err := o.command(w, stkReadPage, []byte{byte(len(page.data) >> 8), byte(len(page.data)), 'F'}, len(page.data))
	if err != nil {
		return err
	}
	for i := range page.data {
		if got[i] != page.data[i] {
			return fmt.Errorf("Prüfung fehlgeschlagen bei Adresse 0x%04x", page.addr+uint32(i))
		}
	}
	return nil
}

func (o optiboot) command(w wire, cmd byte, args []byte, dataLen int) ([]byte, error) {
	frame := make([]byte, 0, 1+len(args)+1)
	frame = append(frame, cmd)
	frame = append(frame, args...)
	frame = append(frame, stkCRC)
	if err := writeAll(w, frame); err != nil {
		return nil, err
	}
	wait := o.cmdWait
	if wait <= 0 {
		wait = 2 * time.Second
	}
	head, err := readExact(w, 1, wait)
	if err != nil {
		return nil, err
	}
	if head[0] != stkInsync {
		return nil, fmt.Errorf("keine Synchronisation (0x%02x)", head[0])
	}
	payload, err := readExact(w, dataLen, wait)
	if err != nil {
		return nil, err
	}
	ok, err := readExact(w, 1, wait)
	if err != nil {
		return nil, err
	}
	if ok[0] != stkOK {
		return nil, fmt.Errorf("Bootloader meldet 0x%02x", ok[0])
	}
	return payload, nil
}

func writeAll(w wire, p []byte) error {
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

func readExact(w wire, n int, timeout time.Duration) ([]byte, error) {
	if n == 0 {
		return nil, nil
	}
	buf := make([]byte, 0, n)
	deadline := time.Now().Add(timeout)
	tmp := make([]byte, n)
	for len(buf) < n {
		remain := time.Until(deadline)
		if remain <= 0 {
			return nil, errTimeout
		}
		if err := w.SetReadTimeout(remain); err != nil {
			return nil, err
		}
		k, err := w.Read(tmp[:n-len(buf)])
		if k > 0 {
			buf = append(buf, tmp[:k]...)
		}
		if len(buf) >= n {
			return buf, nil
		}
		if err != nil {
			if isTimeout(err) {
				return nil, errTimeout
			}
			return nil, err
		}
		if k == 0 {
			return nil, errTimeout
		}
	}
	return buf, nil
}

func isTimeout(err error) bool {
	if err == nil {
		return false
	}
	if errors.Is(err, os.ErrDeadlineExceeded) || errors.Is(err, errTimeout) || errors.Is(err, context.DeadlineExceeded) {
		return true
	}
	var te interface{ Timeout() bool }
	return errors.As(err, &te) && te.Timeout()
}

func sleep(d time.Duration) {
	if d > 0 {
		time.Sleep(d)
	}
}
