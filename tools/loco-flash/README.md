# loco-flash

Writes a Loco-CAN module over an FTDI adapter. Firmware comes from GitHub Releases of [loco-can/loco-can-software](https://github.com/loco-can/loco-can-software/releases). The PC does not need PlatformIO or avrdude. The Optiboot programmer (STK500v1, DTR reset) is inside the program.

V2 modules are an ATmega328P with Optiboot. The adapter's DTR line has to reach RESET, usually through 100 nF. The first attempt uses 115200 baud. If the bootloader does not answer, 57600 is tried once for an old Nano bootloader. Close any serial monitor on that port before writing.

```bash
go build -o loco-flash ./tools/loco-flash

./loco-flash
./loco-flash ports
./loco-flash ls
./loco-flash write -module controller -hardware 2.1 -port /dev/ttyUSB0
./loco-flash write -module switch -hardware 2.1 -all-ftdi
./loco-flash write -file controller_v2_1_0.hex -port COM3
```

With no arguments the workshop window opens in the browser. `./package.sh` builds standalone archives for Windows, Linux, and macOS. Each archive is one program file. Double-click `LocoFlash.exe` on Windows. On Linux and macOS run `./LocoFlash`.

`-hardware 2.1` selects the highest patch in the newest release that has that board, for example 2.1.12. `-hardware 2.1.0` selects that patch. Pre-releases are included. `-stable` skips them. `-tag controller` pins one release.

`-dry-run` downloads the file and prints the built-in write step. `-avrdude /path/to/avrdude` uses an external avrdude instead. Several FTDI adapters (`USB VID 0403`) are written one after another with `-all-ftdi`.

ESP32 `.bin` files are listed by `ls` and refused by `write`. They are not programmed through this FTDI path.

A release may include `manifest.json`. Without it, file names such as `controller_v2_1_0.hex` are enough. Build a manifest from a directory of those files:

```bash
./loco-flash manifest -dir dist -out dist/manifest.json
```
