//go:build linux

package main

import (
	"os"
	"path/filepath"
)

// FTDI's default 16 ms latency timer delays the DTR pulse and STK500
// replies. avrdude sets ASYNC_LOW_LATENCY; writing 1 ms is the sysfs
// equivalent and is ignored when the file is missing or not writable.
func prepareSerial(portName string) {
	base := filepath.Base(portName)
	candidates := []string{
		filepath.Join("/sys/bus/usb-serial/devices", base, "latency_timer"),
		filepath.Join("/sys/class/tty", base, "device", "latency_timer"),
	}
	for _, path := range candidates {
		if err := os.WriteFile(path, []byte("1"), 0644); err == nil {
			return
		}
	}
}
