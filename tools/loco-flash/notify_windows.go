//go:build windows

package main

import "golang.org/x/sys/windows"

func notify(title, text string) {
	windows.MessageBox(0,
		windows.StringToUTF16Ptr(text),
		windows.StringToUTF16Ptr(title),
		windows.MB_OK|windows.MB_ICONINFORMATION)
}
