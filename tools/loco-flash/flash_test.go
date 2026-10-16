package main

import (
	"context"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"testing"
	"time"
)

func TestBaudFallback(t *testing.T) {
	dir := t.TempDir()
	logPath := filepath.Join(dir, "bauds")
	script := filepath.Join(dir, "avrdude")
	body := `#!/bin/sh
prev=
baud=
for a in "$@"; do
  if [ "$prev" = "-b" ]; then
    baud=$a
  fi
  prev=$a
done
echo "$baud" >> "` + logPath + `"
if [ "$baud" = "115200" ]; then
  exit 1
fi
exit 0
`
	if err := os.WriteFile(script, []byte(body), 0o755); err != nil {
		t.Fatal(err)
	}
	err := flashHex(context.Background(), execRunner{}, script, "atmega328p", "/dev/ttyUSB0", "controller.hex", []int{115200, 57600}, time.Second)
	if err != nil {
		t.Fatal(err)
	}
	got, err := os.ReadFile(logPath)
	if err != nil {
		t.Fatal(err)
	}
	if string(got) != "115200\n57600\n" {
		t.Fatalf("bauds %q", got)
	}
}

func TestExplicitBaudDoesNotRetry(t *testing.T) {
	if !baudSetOnce(t, []int{57600}) {
		t.Fatal("expected a single attempt")
	}
	got := baudAttempts(57600, true)
	if len(got) != 1 || got[0] != 57600 {
		t.Fatalf("bauds %v", got)
	}
	def := baudAttempts(115200, false)
	if joinInts(def) != "115200,57600" {
		t.Fatalf("default bauds %v", def)
	}
}

func TestFindPlatformIOAvrdude(t *testing.T) {
	home := t.TempDir()
	t.Setenv("HOME", home)
	t.Setenv("PATH", home)
	t.Setenv("AVRDUDE", "")
	dir := filepath.Join(home, ".platformio", "packages", "tool-avrdude")
	if err := os.MkdirAll(dir, 0o755); err != nil {
		t.Fatal(err)
	}
	bin := filepath.Join(dir, "avrdude")
	if err := os.WriteFile(bin, []byte("#!/bin/sh\n"), 0o755); err != nil {
		t.Fatal(err)
	}
	got, err := findAvrdude("")
	if err != nil {
		t.Fatal(err)
	}
	if got != bin {
		t.Fatalf("got %s want %s", got, bin)
	}
}

func TestAvrdudeUsesOptiboot(t *testing.T) {
	args := avrdudeArgs("", "atmega328p", "/dev/ttyUSB0", "controller_v2_1_0.hex", 115200)
	line := strings.Join(args, " ")
	for _, want := range []string{"-p m328p", "-c arduino", "-P /dev/ttyUSB0", "-b 115200", "-D", "flash:w:controller_v2_1_0.hex:i"} {
		if !strings.Contains(line, want) {
			t.Fatalf("missing %s in %s", want, line)
		}
	}
}

func baudSetOnce(t *testing.T, bauds []int) bool {
	t.Helper()
	dir := t.TempDir()
	logPath := filepath.Join(dir, "bauds")
	script := filepath.Join(dir, "avrdude")
	body := "#!/bin/sh\necho x >> '" + logPath + "'\nexit 1\n"
	if err := os.WriteFile(script, []byte(body), 0o755); err != nil {
		t.Fatal(err)
	}
	_ = flashHex(context.Background(), execRunner{}, script, "m328p", "COM3", "a.hex", bauds, time.Second)
	got, err := os.ReadFile(logPath)
	if err != nil {
		t.Fatal(err)
	}
	return strings.Count(string(got), "x") == 1
}

func joinInts(v []int) string {
	parts := make([]string, len(v))
	for i, n := range v {
		parts[i] = strconv.Itoa(n)
	}
	return strings.Join(parts, ",")
}
