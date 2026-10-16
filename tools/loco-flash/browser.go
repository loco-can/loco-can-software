package main

import (
	"fmt"
	"os/exec"
	"runtime"
)

func openBrowser(url string) error {
	switch runtime.GOOS {
	case "windows":
		return exec.Command("rundll32", "url.dll,FileProtocolHandler", url).Start()
	case "darwin":
		return exec.Command("open", url).Start()
	default:
		for _, bin := range []string{"xdg-open", "gio", "sensible-browser"} {
			path, err := exec.LookPath(bin)
			if err != nil {
				continue
			}
			if bin == "gio" {
				return exec.Command(path, "open", url).Start()
			}
			return exec.Command(path, url).Start()
		}
		return fmt.Errorf("kein Programm zum Öffnen des Browsers gefunden")
	}
}
