package main

import (
	"context"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"time"
)

type commandRunner interface {
	Run(ctx context.Context, name string, args []string) error
}

type execRunner struct{}

func (execRunner) Run(ctx context.Context, name string, args []string) error {
	cmd := exec.CommandContext(ctx, name, args...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func findAvrdude(explicit string) (string, error) {
	if explicit != "" {
		if _, err := os.Stat(explicit); err != nil {
			return "", fmt.Errorf("avrdude %s: %w", explicit, err)
		}
		return explicit, nil
	}
	if env := os.Getenv("AVRDUDE"); env != "" {
		if _, err := os.Stat(env); err != nil {
			return "", fmt.Errorf("avrdude %s: %w", env, err)
		}
		return env, nil
	}
	path, err := exec.LookPath("avrdude")
	if err != nil {
		return "", fmt.Errorf("avrdude not found on PATH (install avrdude, or pass -avrdude)")
	}
	return path, nil
}

func avrdudeConf(bin string) string {
	dir := filepath.Dir(bin)
	candidates := []string{
		filepath.Join(dir, "avrdude.conf"),
		filepath.Join(dir, "..", "etc", "avrdude.conf"),
	}
	for _, path := range candidates {
		st, err := os.Stat(path)
		if err == nil && !st.IsDir() {
			return path
		}
	}
	return ""
}

func avrdudeArgs(conf, mcu, port, file string, baud int) []string {
	args := []string{}
	if conf != "" {
		args = append(args, "-C", conf)
	}
	if mcu == "" {
		mcu = "m328p"
	}
	if mcu == "atmega328p" {
		mcu = "m328p"
	}
	args = append(args,
		"-p", mcu,
		"-c", "arduino",
		"-P", port,
		"-b", fmt.Sprintf("%d", baud),
		"-D",
		"-U", "flash:w:"+file+":i",
	)
	return args
}

func baudAttempts(baud int, baudSet bool) []int {
	if baudSet {
		if baud == 0 {
			baud = 115200
		}
		return []int{baud}
	}
	return []int{115200, 57600}
}

func flashHex(ctx context.Context, run commandRunner, avrdude, mcu, port, file string, bauds []int, timeout time.Duration) error {
	if len(bauds) == 0 {
		return fmt.Errorf("no baud rate")
	}
	conf := avrdudeConf(avrdude)
	var last error
	for i, baud := range bauds {
		if i > 0 {
			fmt.Fprintf(os.Stderr, "no contact at %d, trying %d (old Nano bootloader)\n", bauds[i-1], baud)
		}
		args := avrdudeArgs(conf, mcu, port, file, baud)
		fmt.Fprintf(os.Stderr, "%s %s\n", avrdude, joinArgs(args))
		attempt, cancel := context.WithTimeout(ctx, timeout)
		err := run.Run(attempt, avrdude, args)
		cancel()
		if err == nil {
			return nil
		}
		if ctx.Err() != nil {
			return ctx.Err()
		}
		last = err
	}
	return fmt.Errorf("flash %s: %w", port, last)
}

func joinArgs(args []string) string {
	out := ""
	for i, arg := range args {
		if i > 0 {
			out += " "
		}
		if needsQuote(arg) {
			out += fmt.Sprintf("%q", arg)
		} else {
			out += arg
		}
	}
	return out
}

func needsQuote(s string) bool {
	for _, r := range s {
		if r == ' ' || r == '\t' {
			return true
		}
	}
	return false
}
