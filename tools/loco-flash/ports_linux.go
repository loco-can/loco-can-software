//go:build linux

package main

import (
	"os"
	"path/filepath"
	"strings"
)

func listPorts() ([]Port, error) {
	return portsFromSysfs("/sys")
}

func portsFromSysfs(root string) ([]Port, error) {
	matches, err := filepath.Glob(filepath.Join(root, "class", "tty", "tty*"))
	if err != nil {
		return nil, err
	}
	var ports []Port
	for _, ttyPath := range matches {
		name := filepath.Base(ttyPath)
		if !strings.HasPrefix(name, "ttyUSB") && !strings.HasPrefix(name, "ttyACM") {
			continue
		}
		vid, pid, desc, ok := usbIdentity(filepath.Join(ttyPath, "device"))
		if !ok || !strings.EqualFold(vid, "0403") {
			continue
		}
		ports = append(ports, Port{
			Name: "/dev/" + name,
			USB:  strings.ToLower(vid) + ":" + strings.ToLower(pid),
			Desc: desc,
		})
	}
	sortPorts(ports)
	return ports, nil
}

func usbIdentity(deviceLink string) (vid, pid, desc string, ok bool) {
	dir, err := filepath.EvalSymlinks(deviceLink)
	if err != nil {
		return "", "", "", false
	}
	for i := 0; i < 10; i++ {
		vendor, errV := os.ReadFile(filepath.Join(dir, "idVendor"))
		product, errP := os.ReadFile(filepath.Join(dir, "idProduct"))
		if errV == nil && errP == nil {
			man := readTrim(filepath.Join(dir, "manufacturer"))
			prod := readTrim(filepath.Join(dir, "product"))
			return strings.TrimSpace(string(vendor)), strings.TrimSpace(string(product)), joinDesc(man, prod), true
		}
		parent := filepath.Dir(dir)
		if parent == dir {
			break
		}
		dir = parent
	}
	return "", "", "", false
}

func readTrim(path string) string {
	b, err := os.ReadFile(path)
	if err != nil {
		return ""
	}
	return strings.TrimSpace(string(b))
}

func joinDesc(manufacturer, product string) string {
	if manufacturer == "" {
		return product
	}
	if product == "" || strings.Contains(strings.ToLower(product), strings.ToLower(manufacturer)) {
		if product != "" {
			return product
		}
		return manufacturer
	}
	return manufacturer + " " + product
}
