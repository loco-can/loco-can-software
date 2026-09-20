# Loco-CAN Programmer

The programmer is a module that is used to analyse the CAN communication and edit settings of the different modules. It is based on a ESP32 with a color OLED Display and a rotary encoder with a pushbutton for controlling the functions. The connection to the bus is compatible with all Loco-CAN modules.

In addition a webserver hosted on the ESP32, using a WIFI accesspoint, gives an more powerfull access to all internal functions. The webservice is responsive and can be used on smartphones, tablets and computers. All functions can be used locally and via the webservice.

Join **LOCO-CAN-PRG** (password `lococanprg`) and open [http://192.168.4.1](http://192.168.4.1). The packed Next.js UI lives in LittleFS; CAN frames are relayed through `/api/status`, `/api/frames` and `/api/tx`.

The programmer has two modes, to analyse the CAN bus and to setup modules.

## CAN analysis mode

The programmer starts in the analysis mode. It scans the bus and shows the messages that are received. Messages with the same ID and UUID are summoned and only shown once. The type of the received message (its message ID) is translated to plain text. The transported values are formatted with a plain text label-value pair depending on the message type. Integer values, like speed or throttle, are shown as integer numbers, boolen values in a flag byte as switches.

## Module setup

In the setup mode all parameters of the connected modules can be set. The modules are pinged to collect informations about the type and UUID of the modules. Formatting and plain text labels are used for simple identification like in analysis mode.

A list shows the connected modules. When selecting a module, the available parameters are shown in the way of the analysis mode, can be modified and written back.

## Drop-in for loco-can-software

Copy this folder to `src/module/programmer/` of [loco-can-software](https://github.com/loco-can/loco-can-software).

In `src/config.h`:

```c
#define PROGRAMMER_MODULE "src/module/programmer/config.h"
```

In `hardware.h` select the programmer instead of the controller:

```c
#define MODULE PROGRAMMER_MODULE
#define PROGRAMMER_MODULE_VERSION V_3_0
```

In `src/LocoCANcore.h` add:

```c
#ifdef MODULE_PROGRAMMER_H
    MODULE_PROGRAMMER _module;
#endif
```

Board: ESP32 / ESP32-S3, Arduino core. Pins are in `config.h` (CAN 17/18, ST7789 SPI, KY-040 encoder).

## Pack and upload the webservice

From the repository root:

```bash
npm run firmware
```

That writes a gzipped static export to `firmware/module/programmer/data/`. Copy that folder to `data/` next to `loco-can-software.ino` (Arduino LittleFS looks next to the sketch, not inside the module). Then upload the filesystem:

- Arduino IDE: **Tools → ESP32 Sketch Data Upload** (LittleFS)
- PlatformIO: set `board_build.filesystem = littlefs` and run `pio run -t uploadfs`

Reboot the programmer, join **LOCO-CAN-PRG** / `lococanprg`, and open http://192.168.4.1. If LittleFS is empty the access point still starts and shows a short setup page.

WPA2 needs eight or more characters; the password is `lococanprg`.

