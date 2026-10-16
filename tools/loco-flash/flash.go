package main

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"
)

var errNoAvrdude = errors.New("avrdude not found")

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
		st, err := os.Stat(explicit)
		if err != nil {
			return "", fmt.Errorf("avrdude %s: %w", explicit, err)
		}
		if st.IsDir() {
			return "", fmt.Errorf("avrdude %s is a directory", explicit)
		}
		return explicit, nil
	}
	for _, path := range avrdudeCandidates("") {
		st, err := os.Stat(path)
		if err == nil && !st.IsDir() {
			return path, nil
		}
	}
	return "", errNoAvrdude
}

func avrdudeCandidates(explicit string) []string {
	var out []string
	if explicit != "" {
		out = append(out, explicit)
	}
	if env := os.Getenv("AVRDUDE"); env != "" {
		out = append(out, env)
	}
	if path, err := exec.LookPath("avrdude"); err == nil {
		out = append(out, path)
	}
	home, err := os.UserHomeDir()
	if err != nil {
		return out
	}
	pio := filepath.Join(home, ".platformio", "packages")
	for _, name := range []string{"avrdude", "avrdude.exe"} {
		matches, _ := filepath.Glob(filepath.Join(pio, "tool-avrdude*", name))
		out = append(out, matches...)
	}
	return out
}

func flashWithAvrdude(ctx context.Context, port string, data []byte, bauds []int, log flashLogger) error {
	if log == nil {
		log = func(string, int, int) {}
	}
	dude, err := findAvrdude("")
	if err != nil {
		return err
	}
	tmp, err := os.CreateTemp("", "loco-flash-*.hex")
	if err != nil {
		return err
	}
	name := tmp.Name()
	defer os.Remove(name)
	if _, err := tmp.Write(data); err != nil {
		tmp.Close()
		return err
	}
	if err := tmp.Close(); err != nil {
		return err
	}
	log("avrdude "+dude, 0, 0)
	return flashHex(ctx, logRunner{log: log}, dude, "atmega328p", port, name, bauds, 45*time.Second)
}

type logRunner struct {
	log flashLogger
}

func (r logRunner) Run(ctx context.Context, name string, args []string) error {
	cmd := exec.CommandContext(ctx, name, args...)
	var buf bytes.Buffer
	cmd.Stdout = io.MultiWriter(os.Stdout, &buf)
	cmd.Stderr = io.MultiWriter(os.Stderr, &buf)
	err := cmd.Run()
	for _, line := range strings.Split(buf.String(), "\n") {
		line = strings.TrimSpace(line)
		if line != "" && r.log != nil {
			r.log(line, 0, 0)
		}
	}
	return err
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
