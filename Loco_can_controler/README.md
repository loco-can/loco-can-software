# LOCO-Controler
The LOCO-Controler software is used in the Universal and Universal-WiFi modules. It is used to create a driving console. It can directly be plugged into the Universal Control Adapter that provides JST-connectors for input and ouput devices.

## Hardware version
The hardware specifications are described in the README files of the Universal and Universal-WiFi modules.

## IO-connectors
| Bus 1         |
|:--------------|
| 1 VCC         |
| 2 LED red     |
| 3 LED green   |
| 4 Instr.light |
| 5 Horn        |
| 6 Volt Batt   |
| 7 Volt Motor  |
| 8 Ampere      |
| 9 GND         |

| Bus 2a       |
|:-------------|
| 1 +12V       |
| 2 Light SW   |
| 3 Mains SW   |
| 4 Dir. SW    |
| 5 Light SW 1 |

| Bus 2b      |
|:------------|
| 1 Break pot |
| 2 Drive pot |
| 3 Horn 1    |
| 4 GND       |

## Analog switches
The input pins for light, mains and direction use the analog switch library. The output values are set by learned voltages, that represent the different switch positions. On the hardware side a switch witch resistors can be used.

## Measurement Outputs
The outputs to display voltage or current values can be set to analog or servo signals (analog output is not yet implemented).