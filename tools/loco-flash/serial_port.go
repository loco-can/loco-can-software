//go:build !linux

package main

import (
	"fmt"
	"time"

	"go.bug.st/serial"
)

// serialWire adapts go.bug.st/serial to wire. Read returns (0, nil) on timeout.
type serialWire struct {
	port serial.Port
}

func openSerial(portName string, baud int) (*serialWire, error) {
	if baud == 0 {
		baud = 115200
	}
	prepareSerial(portName)
	mode := &serial.Mode{
		BaudRate: baud,
		DataBits: 8,
		Parity:   serial.NoParity,
		StopBits: serial.OneStopBit,
		// Start with DTR and RTS released. The reset pulse is explicit.
		// Unix still glitches the lines high for a few milliseconds on open.
		InitialStatusBits: &serial.ModemOutputBits{DTR: false, RTS: false},
	}
	port, err := serial.Open(portName, mode)
	if err != nil {
		return nil, fmt.Errorf("%s: %w", portName, err)
	}
	if err := port.SetReadTimeout(50 * time.Millisecond); err != nil {
		port.Close()
		return nil, err
	}
	return &serialWire{port: port}, nil
}

func (s *serialWire) Read(p []byte) (int, error)  { return s.port.Read(p) }
func (s *serialWire) Write(p []byte) (int, error) { return s.port.Write(p) }
func (s *serialWire) SetDTR(v bool) error         { return s.port.SetDTR(v) }
func (s *serialWire) SetRTS(v bool) error         { return s.port.SetRTS(v) }
func (s *serialWire) SetReadTimeout(d time.Duration) error {
	return s.port.SetReadTimeout(d)
}
func (s *serialWire) ResetInputBuffer() error { return s.port.ResetInputBuffer() }
func (s *serialWire) Drain() error            { return s.port.Drain() }
func (s *serialWire) Close() error            { return s.port.Close() }
