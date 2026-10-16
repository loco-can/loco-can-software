//go:build !linux && !darwin && !windows

package main

import "fmt"

func listPorts() ([]Port, error) {
	return nil, fmt.Errorf("listing FTDI ports is not supported on this OS; pass -port")
}
