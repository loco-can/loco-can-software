# LOCO-Controller
The LOCO-Controller software is used in the Universal and Universal-WiFi modules to create a driving console. It can directly be plugged into the Universal Control Adapter that provides JST-connectors for input and ouput devices.

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

## IO

### Analog switches
The input pins for light, mains and direction use the analog switch library. The output values are set by learned voltages, that represent the different switch positions. On the hardware side a switch witch resistors can be used.

### Measurement Outputs
The outputs to display voltage or current values can be set to analog or servo signals (analog output is not yet implemented).

## Functions

### Locomotive Setup
The locomotive setup process is initiated by setting the main switch to off, pushing and holding the horn button and switching on the mains again. The status led will alternatingly blink orange and green and the headlights of the locomotive will start to blink on both sides. If there are more locomotives connected to the bus, a short push on the horn will switch to the next locomotive and this locos lights will start to blink.

The setup mode is ended by switching the main switch to off.

#### Pairing
A controller can be used for all driving modules (like motor or servo modules) but also paired with a particular module. In this case the uuid of the paired module is saved in the oposite unit (uuid of the motor in the controller and vice versa).

After entering the setup mode the direction switch has to be in the neutral position. If the locomotive already is paired, the status led will blink green only. The pairing is finished by holding the horn button for more than three seconds. The status led and the locomotive headlights will rapidly blink green three times. If the locomotive already is paired, the pairing is deleted and the status led will blink alteratingly green and orange again.

#### Direction setup
Depending on the side, the train is connected to the locomotive, the direction settings on the controller can vary from the actual driving direction. The can be set using the locomotive setup process.

After entering the setup mode and selecting the locomotive by pressing the horn button all headlights of the locomotive will blink. By switching the direction button to forward or reverse, the corresponding lights on the locomotive will blink, the other side will be off. By setting the direction switch to the position, where the forward lights are blinking and holding the horn button for three seconds, the forward direction is set.

This operation is necessary in case of multiple traction.

### Multi Traction
The controller module takes the following actions:

1. Main on
2. Registers all motor modules using the module heartbeat
3. Registers all controllers by the module heartbeat

## How-To???

C1 = Controller-1
M1 = Motor-1

C1 (active)
C2 (active paired)
C3
C4 (paired)

M1
M2 (paired->Controller-2) - one lococ
M3 (paired->Controller-2) - one lococ
M4 (paired->Controller-4)
M5 (paired->Controller-5 - not present)

C1 -> M1, M4, M5
C2 -> M2, M3

deactivate C2
C1 -> takes M2, M3

### Activate controller

Multiple motors can be paired to a controller. A Controller has no pairing information stored. In case of pairing the controller sets the paired flag in the drive message. The paired flag of the motor is set, if a pairing uuid is stored. 

**Controller finds paired motors**
* paired flag in drive message is true
* show paired status led???

**Controller finds no paired motor** 
* paired flag in drive message is true




* controller is not paired -> get all not paired motor modules -> all stati must be ready to drive
* Controller is paired -> get paired motor module -> only paired status must be ready to drive

* Motor module checks, if the paired controller is active -> sets paired flag in motor status
not paired: listen to not paired controller or 


```mermaid
  graph TD;
      A-->B;
      A-->C;
      B-->D;
      C-->D;
```
