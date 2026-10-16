//go:build darwin

package main

import "path/filepath"

func listPorts() ([]Port, error) {
	matches, err := filepath.Glob("/dev/cu.usbserial*")
	if err != nil {
		return nil, err
	}
	more, err := filepath.Glob("/dev/cu.FT*")
	if err != nil {
		return nil, err
	}
	matches = append(matches, more...)
	var ports []Port
	seen := map[string]bool{}
	for _, name := range matches {
		if seen[name] || !isFTDIDeviceName(name) {
			continue
		}
		seen[name] = true
		ports = append(ports, Port{Name: name, USB: "0403", Desc: "FTDI"})
	}
	sortPorts(ports)
	return ports, nil
}
