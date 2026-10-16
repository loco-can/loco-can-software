# Loco-CAN WIFI

The WIFI module is an ESP32 CAN node that also pairs with a second WIFI
module over ESP-NOW. Together the two boards act as a wireless CAN cable
when the vehicles (or two bus segments) are not joined by a physical
CAN wire.

## Behaviour

1. Each board starts the standard Loco-CAN TWAI interface (500 kbit/s).
2. ESP-NOW runs in station mode on channel 1 and broadcasts a hello.
3. The first other WIFI module that answers becomes the peer.
4. Frames heard on the local CAN bus are sent to that peer.
5. Frames received over ESP-NOW are injected onto the local CAN bus
   **only while no physical cable is present**.

Cable detection uses coincidence: if the same CAN identifier, UUID and
payload appear both on the wire and on ESP-NOW, both modules sit on the
same bus. Bridging is then disabled so the frames are not looped. When
the cable is unplugged the coincidence stops and bridging starts again.

The module announces itself with `CAN_ID_MODULE_HEARTBEAT` (type `0x81`,
wireless bit set).

Flash the same `wifi_v2` firmware on both ESP32 boards.

## Pins (V2.0 / ESP32-WROOM-32)

| Function | GPIO |
| --- | --- |
| CAN RX | 16 |
| CAN TX | 17 |
| CAN status LED | 2 |

## PlatformIO

```bash
pio run -e wifi_v2
pio run -e wifi_v2 -t upload
```

## Arduino IDE

In `hardware.h` select:

```c
#define MODULE WIFI_MODULE
#define WIFI_MODULE_VERSION V_2_0
```

Board: ESP32-WROOM-32. The core includes the module when `MODULE_WIFI_H` is
defined.
