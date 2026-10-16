//go:build linux

package main

import (
	"fmt"
	"time"

	"golang.org/x/sys/unix"
)

// posixWire talks to the FTDI adapter with the same ioctls as avrdude.
// DTR and RTS are written in one TIOCMSET; separate SetDTR/SetRTS calls
// glitch the other line and miss the Optiboot window on Linux.
type posixWire struct {
	fd          int
	readTimeout time.Duration
}

func openSerial(portName string, baud int) (*posixWire, error) {
	if baud == 0 {
		baud = 115200
	}
	prepareSerial(portName)
	fd, err := unix.Open(portName, unix.O_RDWR|unix.O_NOCTTY|unix.O_CLOEXEC, 0)
	if err != nil {
		return nil, fmt.Errorf("%s: %w", portName, err)
	}
	if err := unix.SetNonblock(fd, true); err != nil {
		unix.Close(fd)
		return nil, err
	}
	speed := unix.B115200
	if baud == 57600 {
		speed = unix.B57600
	}
	tio := unix.Termios{}
	tio.Cflag = uint32(unix.CS8 | unix.CREAD | unix.CLOCAL | speed)
	tio.Ispeed = uint32(speed)
	tio.Ospeed = uint32(speed)
	tio.Cc[unix.VMIN] = 0
	tio.Cc[unix.VTIME] = 0
	if err := unix.IoctlSetTermios(fd, unix.TCSETS, &tio); err != nil {
		unix.Close(fd)
		return nil, err
	}
	_ = unix.IoctlSetInt(fd, unix.TIOCEXCL, 0)
	w := &posixWire{fd: fd, readTimeout: 50 * time.Millisecond}
	if err := w.SetLines(false, false); err != nil {
		w.Close()
		return nil, err
	}
	return w, nil
}

func (p *posixWire) Read(b []byte) (int, error) {
	timeout := p.readTimeout
	if timeout <= 0 {
		timeout = 50 * time.Millisecond
	}
	deadline := time.Now().Add(timeout)
	for {
		remain := time.Until(deadline)
		if remain <= 0 {
			return 0, nil
		}
		tv := unix.NsecToTimeval(remain.Nanoseconds())
		var r unix.FdSet
		r.Set(p.fd)
		n, err := unix.Select(p.fd+1, &r, nil, nil, &tv)
		if err == unix.EINTR {
			continue
		}
		if err != nil {
			return 0, err
		}
		if n == 0 || !r.IsSet(p.fd) {
			return 0, nil
		}
		k, err := unix.Read(p.fd, b)
		if err == unix.EINTR {
			continue
		}
		if k < 0 {
			k = 0
		}
		return k, err
	}
}

func (p *posixWire) Write(b []byte) (int, error) {
	n, err := unix.Write(p.fd, b)
	if n < 0 {
		n = 0
	}
	return n, err
}

func (p *posixWire) SetDTR(v bool) error {
	status, err := unix.IoctlGetInt(p.fd, unix.TIOCMGET)
	if err != nil {
		return err
	}
	if v {
		status |= unix.TIOCM_DTR
	} else {
		status &^= unix.TIOCM_DTR
	}
	return unix.IoctlSetPointerInt(p.fd, unix.TIOCMSET, status)
}

func (p *posixWire) SetRTS(v bool) error {
	status, err := unix.IoctlGetInt(p.fd, unix.TIOCMGET)
	if err != nil {
		return err
	}
	if v {
		status |= unix.TIOCM_RTS
	} else {
		status &^= unix.TIOCM_RTS
	}
	return unix.IoctlSetPointerInt(p.fd, unix.TIOCMSET, status)
}

func (p *posixWire) SetLines(dtr, rts bool) error {
	status, err := unix.IoctlGetInt(p.fd, unix.TIOCMGET)
	if err != nil {
		return err
	}
	if dtr {
		status |= unix.TIOCM_DTR
	} else {
		status &^= unix.TIOCM_DTR
	}
	if rts {
		status |= unix.TIOCM_RTS
	} else {
		status &^= unix.TIOCM_RTS
	}
	return unix.IoctlSetPointerInt(p.fd, unix.TIOCMSET, status)
}

func (p *posixWire) SetReadTimeout(d time.Duration) error {
	p.readTimeout = d
	return nil
}

func (p *posixWire) ResetInputBuffer() error {
	return unix.IoctlSetInt(p.fd, unix.TCFLSH, unix.TCIFLUSH)
}

func (p *posixWire) Drain() error {
	return unix.IoctlSetInt(p.fd, unix.TCSBRK, 1)
}

func (p *posixWire) Close() error {
	_ = unix.IoctlSetInt(p.fd, unix.TIOCNXCL, 0)
	return unix.Close(p.fd)
}
