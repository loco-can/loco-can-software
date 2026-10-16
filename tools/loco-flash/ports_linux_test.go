//go:build linux

package main

import (
	"os"
	"path/filepath"
	"testing"
)

func TestPortsFromSysfs(t *testing.T) {
	root := t.TempDir()
	ftdi := filepath.Join(root, "devices", "usb", "1-1")
	iface := filepath.Join(ftdi, "1-1:1.0")
	if err := os.MkdirAll(iface, 0o755); err != nil {
		t.Fatal(err)
	}
	write := func(path, body string) {
		t.Helper()
		if err := os.WriteFile(path, []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	write(filepath.Join(ftdi, "idVendor"), "0403\n")
	write(filepath.Join(ftdi, "idProduct"), "6001\n")
	write(filepath.Join(ftdi, "manufacturer"), "FTDI\n")
	write(filepath.Join(ftdi, "product"), "FT232R USB UART\n")

	other := filepath.Join(root, "devices", "usb", "1-2")
	otherIface := filepath.Join(other, "1-2:1.0")
	if err := os.MkdirAll(otherIface, 0o755); err != nil {
		t.Fatal(err)
	}
	write(filepath.Join(other, "idVendor"), "1a86\n")
	write(filepath.Join(other, "idProduct"), "7523\n")

	link := func(name, target string) {
		t.Helper()
		dir := filepath.Join(root, "class", "tty", name)
		if err := os.MkdirAll(dir, 0o755); err != nil {
			t.Fatal(err)
		}
		if err := os.Symlink(target, filepath.Join(dir, "device")); err != nil {
			t.Fatal(err)
		}
	}
	link("ttyUSB0", iface)
	link("ttyUSB1", otherIface)

	ports, err := portsFromSysfs(root)
	if err != nil {
		t.Fatal(err)
	}
	if len(ports) != 1 {
		t.Fatalf("ports %+v", ports)
	}
	if ports[0].Name != "/dev/ttyUSB0" || ports[0].USB != "0403:6001" {
		t.Fatalf("port %+v", ports[0])
	}
	if ports[0].Desc != "FTDI FT232R USB UART" {
		t.Fatalf("desc %q", ports[0].Desc)
	}
}
