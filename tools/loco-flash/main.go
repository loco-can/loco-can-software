package main

import (
	"context"
	"flag"
	"fmt"
	"io"
	"os"
	"os/signal"
	"strings"
	"time"
)

const usageText = `loco-flash downloads Loco-CAN firmware from GitHub releases and writes it
through an FTDI adapter. The Optiboot programmer is built in: 115200 baud,
DTR resets the board (usually via 100 nF). If that does not answer,
57600 is tried once for the old Nano bootloader. No install is required.

  loco-flash
  loco-flash ports
  loco-flash identify -port /dev/ttyUSB0
  loco-flash ls
  loco-flash write -module controller -hardware 2.1 -port /dev/ttyUSB0
  loco-flash write -module switch -hardware 2.1 -all-ftdi
  loco-flash write -file controller_v2_1_0.hex -port COM3
  loco-flash manifest -dir dist -out dist/manifest.json

With no arguments, loco-flash opens the workshop window. The newest
release that contains a matching .hex is used, including pre-releases.
-hardware 2.1 takes the highest patch (2.1.12). -hardware 2.1.12 selects
that patch. -stable skips pre-releases. -avrdude uses an external avrdude
instead of the built-in programmer.

ESP32 .bin images are not written over FTDI.
`

func main() {
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt)
	defer stop()

	if len(os.Args) < 2 {
		if err := cmdGUI(ctx, nil); err != nil {
			fmt.Fprintln(os.Stderr, err)
			notify("Loco-CAN", err.Error())
			os.Exit(1)
		}
		return
	}
	if os.Args[1] == "-h" || os.Args[1] == "--help" || os.Args[1] == "help" {
		fmt.Fprint(os.Stdout, usageText)
		return
	}

	var err error
	switch os.Args[1] {
	case "ports":
		err = cmdPorts()
	case "identify":
		err = cmdIdentify(ctx, os.Args[2:])
	case "ls":
		err = cmdLS(ctx, os.Args[2:])
	case "write":
		err = cmdWrite(ctx, os.Args[2:])
	case "gui":
		err = cmdGUI(ctx, os.Args[2:])
	case "manifest":
		err = cmdManifest(os.Args[2:])
	default:
		fmt.Fprintf(os.Stderr, "unknown command %q\n\n%s", os.Args[1], usageText)
		os.Exit(2)
	}
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		if os.Args[1] == "gui" {
			notify("Loco-CAN", err.Error())
		}
		os.Exit(1)
	}
}

func cmdIdentify(ctx context.Context, args []string) error {
	fs := flag.NewFlagSet("identify", flag.ContinueOnError)
	fs.SetOutput(io.Discard)
	portName := fs.String("port", "", "serial port of one FTDI adapter")
	if err := fs.Parse(args); err != nil {
		fmt.Fprint(os.Stderr, usageText)
		return err
	}
	if *portName == "" {
		return fmt.Errorf("pass -port")
	}
	info, err := probeModule(ctx, *portName)
	if err != nil {
		return err
	}
	fmt.Printf("%s", info.Module)
	if info.Version != "" {
		fmt.Printf(" %s", info.Version)
	}
	if info.TypeID != 0 {
		fmt.Printf(" id=0x%02x", info.TypeID)
	}
	if info.CAN != nil {
		if *info.CAN {
			fmt.Print(" can=1")
		} else {
			fmt.Print(" can=0")
		}
	}
	fmt.Println()
	if info.Raw != "" {
		fmt.Println(info.Raw)
	}
	return nil
}

func cmdPorts() error {
	ports, err := listPorts()
	if err != nil {
		return err
	}
	if len(ports) == 0 {
		fmt.Println("no FTDI adapters found")
		return nil
	}
	for _, port := range ports {
		fmt.Println(port)
	}
	return nil
}

type releaseFlags struct {
	repo   string
	tag    string
	token  string
	stable bool
}

func (f *releaseFlags) add(fs *flag.FlagSet) {
	fs.StringVar(&f.repo, "repo", defaultRepo, "GitHub repository (owner/name)")
	fs.StringVar(&f.tag, "tag", "", "release tag (default: newest release with firmware)")
	fs.StringVar(&f.token, "token", os.Getenv("GITHUB_TOKEN"), "GitHub token (or GITHUB_TOKEN)")
	fs.BoolVar(&f.stable, "stable", false, "ignore pre-releases")
}

func (f releaseFlags) client() *Client {
	return &Client{Repo: f.repo, Token: f.token}
}

func cmdLS(ctx context.Context, args []string) error {
	fs := flag.NewFlagSet("ls", flag.ContinueOnError)
	fs.SetOutput(io.Discard)
	var rel releaseFlags
	rel.add(fs)
	if err := fs.Parse(args); err != nil {
		fmt.Fprint(os.Stderr, usageText)
		return err
	}
	releases, err := rel.client().Releases(ctx, rel.tag, rel.stable)
	if err != nil {
		return err
	}
	show := releases[0]
	when := show.PublishedAt.Format("2006-01-02")
	kind := "release"
	if show.Prerelease {
		kind = "pre-release"
	}
	title := show.Tag
	if show.Name != "" {
		title = show.Name
	}
	fmt.Printf("%s  %s  %s  %s\n", title, show.Tag, kind, when)
	for _, img := range show.Images {
		fmt.Printf("  %s  %s  %s\n", img.Module, img.Version, img.File)
	}
	return nil
}

func cmdManifest(args []string) error {
	fs := flag.NewFlagSet("manifest", flag.ContinueOnError)
	fs.SetOutput(io.Discard)
	dir := fs.String("dir", ".", "directory of .hex files")
	out := fs.String("out", "", "manifest path (default: stdout)")
	if err := fs.Parse(args); err != nil {
		fmt.Fprint(os.Stderr, usageText)
		return err
	}
	images, err := manifestFromDir(*dir)
	if err != nil {
		return err
	}
	var w io.Writer = os.Stdout
	var file *os.File
	if *out != "" {
		file, err = os.Create(*out)
		if err != nil {
			return err
		}
		defer file.Close()
		w = file
	}
	return writeManifest(w, images)
}

func cmdWrite(ctx context.Context, args []string) error {
	fs := flag.NewFlagSet("write", flag.ContinueOnError)
	fs.SetOutput(io.Discard)
	var rel releaseFlags
	rel.add(fs)
	module := fs.String("module", "", "module name, for example controller")
	hardware := fs.String("hardware", "", "hardware major.minor or exact major.minor.patch")
	file := fs.String("file", "", "local .hex file (skips the GitHub release)")
	portName := fs.String("port", "", "serial port of one FTDI adapter")
	all := fs.Bool("all-ftdi", false, "write the same file to every FTDI adapter")
	baud := fs.Int("baud", 115200, "bootloader baud rate")
	avrdude := fs.String("avrdude", "", "path to avrdude")
	dry := fs.Bool("dry-run", false, "download and print the write step, do not write")
	timeout := fs.Duration("timeout", 45*time.Second, "time limit for one write attempt")
	if err := fs.Parse(args); err != nil {
		fmt.Fprint(os.Stderr, usageText)
		return err
	}
	if *portName != "" && *all {
		return fmt.Errorf("pass either -port or -all-ftdi")
	}
	if *portName == "" && !*all {
		return fmt.Errorf("pass -port or -all-ftdi")
	}

	img, tag, err := resolveImage(ctx, rel, *module, *hardware, *file)
	if err != nil {
		return err
	}
	if err := img.flashable(); err != nil {
		return err
	}

	hexPath := *file
	if hexPath == "" {
		dest, err := cachePath(rel.repo, tag, img.File)
		if err != nil {
			return err
		}
		fmt.Fprintf(os.Stderr, "download %s\n", img.File)
		if err := rel.client().Download(ctx, img, dest); err != nil {
			return err
		}
		hexPath = dest
	}

	var ports []Port
	if *all {
		ports, err = listPorts()
		if err != nil {
			return err
		}
		if len(ports) == 0 {
			return fmt.Errorf("no FTDI adapters found")
		}
	} else {
		ports = []Port{{Name: *portName, Desc: "serial"}}
	}

	baudSet := false
	fs.Visit(func(f *flag.Flag) {
		if f.Name == "baud" {
			baudSet = true
		}
	})
	bauds := baudAttempts(*baud, baudSet)

	var hexData []byte
	if *avrdude == "" {
		hexData, err = os.ReadFile(hexPath)
		if err != nil {
			return err
		}
	}

	if *dry {
		fmt.Printf("%s %s  %s\n", img.Module, img.Version, img.File)
		for _, port := range ports {
			fmt.Printf("%s\n", port.Name)
			if *avrdude != "" {
				fmt.Println(*avrdude + " " + joinArgs(avrdudeArgs(avrdudeConf(*avrdude), img.MCU, port.Name, hexPath, bauds[0])))
			} else {
				fmt.Printf("optiboot %d %s %s\n", bauds[0], port.Name, hexPath)
			}
		}
		if len(bauds) > 1 {
			fmt.Printf("if the bootloader does not answer, baud %d is tried\n", bauds[1])
		}
		return nil
	}

	var failed []string
	for _, port := range ports {
		fmt.Fprintf(os.Stderr, "write %s %s to %s\n", img.Module, img.Version, port.Name)
		attempt, cancel := context.WithTimeout(ctx, *timeout)
		var err error
		if *avrdude != "" {
			tool, findErr := findAvrdude(*avrdude)
			if findErr != nil {
				cancel()
				return findErr
			}
			err = flashHex(attempt, execRunner{}, tool, img.MCU, port.Name, hexPath, bauds, *timeout)
		} else {
			err = programHex(attempt, port.Name, hexData, bauds, func(msg string, _, _ int) {
				if msg != "" {
					fmt.Fprintln(os.Stderr, msg)
				}
			})
		}
		cancel()
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err)
			failed = append(failed, port.Name)
		}
	}
	if len(failed) > 0 {
		return fmt.Errorf("failed on %s", strings.Join(failed, ", "))
	}
	return nil
}

func resolveImage(ctx context.Context, rel releaseFlags, module, hardware, file string) (Image, string, error) {
	if file != "" {
		img, ok := parseFirmwareName(file)
		if !ok {
			img = Image{Module: module, File: file, MCU: "atmega328p", Protocol: "arduino", Baud: 115200}
		}
		if module != "" {
			img.Module = strings.ToLower(module)
		}
		return img, "local", nil
	}
	if module == "" || hardware == "" {
		return Image{}, "", fmt.Errorf("pass -module and -hardware, or -file")
	}
	releases, err := rel.client().Releases(ctx, rel.tag, rel.stable)
	if err != nil {
		return Image{}, "", err
	}
	chosen, img, err := Choose(releases, module, hardware)
	if err != nil {
		return Image{}, "", err
	}
	fmt.Fprintf(os.Stderr, "release %s  %s %s  %s\n", chosen.Tag, img.Module, img.Version, img.File)
	return img, chosen.Tag, nil
}
