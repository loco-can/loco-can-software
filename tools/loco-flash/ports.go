package main

import (
	"fmt"
	"path/filepath"
	"sort"
	"strings"
)

// Port is a USB serial adapter. FTDI adapters use vendor id 0403.
type Port struct {
	Name string
	USB  string
	Desc string
}

func (p Port) String() string {
	desc := p.Desc
	if desc == "" {
		desc = "FTDI"
	}
	if p.USB != "" {
		return fmt.Sprintf("%s  %s  %s", p.Name, p.USB, desc)
	}
	return fmt.Sprintf("%s  %s", p.Name, desc)
}

func sortPorts(ports []Port) {
	sort.Slice(ports, func(i, j int) bool {
		return ports[i].Name < ports[j].Name
	})
}

func isFTDIDeviceName(name string) bool {
	base := strings.ToLower(filepath.Base(name))
	return strings.Contains(base, "usbserial") || strings.HasPrefix(base, "ftdi")
}
