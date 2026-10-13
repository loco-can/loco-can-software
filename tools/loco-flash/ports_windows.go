//go:build windows

package main

import (
	"errors"

	"golang.org/x/sys/windows/registry"
)

func listPorts() ([]Port, error) {
	const root = `SYSTEM\CurrentControlSet\Enum\FTDIBUS`
	key, err := registry.OpenKey(registry.LOCAL_MACHINE, root, registry.ENUMERATE_SUB_KEYS)
	if err != nil {
		if errors.Is(err, registry.ErrNotExist) {
			return nil, nil
		}
		return nil, err
	}
	defer key.Close()

	devices, err := key.ReadSubKeyNames(-1)
	if err != nil {
		return nil, err
	}
	var ports []Port
	seen := map[string]bool{}
	for _, device := range devices {
		devKey, err := registry.OpenKey(key, device, registry.ENUMERATE_SUB_KEYS)
		if err != nil {
			continue
		}
		instances, err := devKey.ReadSubKeyNames(-1)
		devKey.Close()
		if err != nil {
			continue
		}
		for _, instance := range instances {
			paramKey, err := registry.OpenKey(key, device+`\`+instance+`\Device Parameters`, registry.QUERY_VALUE)
			if err != nil {
				continue
			}
			name, _, err := paramKey.GetStringValue("PortName")
			paramKey.Close()
			if err != nil || name == "" || seen[name] {
				continue
			}
			seen[name] = true
			ports = append(ports, Port{Name: name, USB: "0403", Desc: "FTDI"})
		}
	}
	sortPorts(ports)
	return ports, nil
}
