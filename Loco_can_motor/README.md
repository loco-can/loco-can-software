# LOCO-Motor
With the LOCO-Motor module all types of power drivers can be acceesed via the CAN bus. The setup of the motor software can be adapted to a wide variety of motor control drivers. With the additional light and horn outputs a locomotive control can be set up with a control and a motor module.

The internal motor voltage is sent as CAN_ID_MOTOR_VOLTAGE to the bus as well as two battery measurement inputs, that provide the state of up to two batteries connected in series.

## V2.2-s
In the version V2.2 a connector for battery voltage measurement is implemented. The two leads can be used to measure two batteries connected in series.

### Motor driver
Plug: MATE-N-LOK 1-794617-0
Jack: MATE-N-LOK 4-794618-0

| pin | usage       | pin | usage       |
|:----|:------------|:----|:------------|
| 1   | +12V IN     | 2   | Light       |               
| 3   | DRIVE       | 4   | HORN        |
| 5   | BREAK       | 6   | MOTOR VOLT+ |
| 7   | FORWARD     | 8   | MOTOR VOLT- |
| 9   | REVERSE     | 10  | GND         |

### Battery voltage
In the standard settings battery voltage 1 is relative to GND, battery voltage 2 relative to battery 1. The voltage message sends three values for overall voltage (CAN_ID_BATT_VOLTAGE), voltage 1 (CAN_ID_BATT_1_VOLTAGE) and voltage 2 (CAN_ID_BATT_2_VOLTAGE).

Jack: 2-pin JST-PH connector
| pin | usage     |
|:----|:----------|
| 1   | Battery 1 |
| 2   | Battery 2 |

### FTDI Interface
Standard 5-pin FTDI Interface


## V2.1a/V2.1b-s
The versions only differ in the pinout of the FDTI-interface.

### Motor driver
Plug: MATE-N-LOK 1-794617-0
Jack: MATE-N-LOK 4-794618-0

| pin | usage       | pin | usage       |
|:----|:------------|:----|:------------|
| 1   | +12V IN     | 2   | Light       |               
| 3   | DRIVE       | 4   | HORN        |
| 5   | BREAK       | 6   | MOTOR VOLT+ |
| 7   | FORWARD     | 8   | MOTOR VOLT- |
| 9   | REVERSE     | 10  | GND         |

Pins 6 and 8 are connected to the motor output to sense the voltage. Drive and break outputs provide a PWM signal to drive the power output stage.

The outputs on pins 3, 5 and 7 can be configured for PWM or servo motors.

### FTDI Interface
| V2.1a    | V2.1b     |
|:---------|:----------|
| 1 DTS    | **1 DTS** |
| 2 **TX** | **2 RX**  |
| 3 **RX** | **3 TX**  |
| 4 3V3    | **4 3V3** |
| 5 GND    | **5 GND** |

## Motor status

|Bit 7|Bit 6|Bit 5|Bit 4 |Bit 3  |Bit 2|Bit 1|Bit 0|
|-----|-----|-----|------|-------|-----|-----|-----|
|error|ready|stop |paired|reverse|dir  |drive|mains|

* error: the module reports an error
* ready: ready to drive (mains on, direction selected)
* stop: the locomotive is standing
* reverse: the direction works the other way around 
* paired: the drive module is paired with a controller, the uuid of the paired controller is stored in the module
* dir: the selected direction (0: forware, 1: reward)
* drive: motor drive enabled (direction in neutral)
* mains: the main switch status (0: off, 1: on)


## Direction reverse
The direction assignment can be reversed with a flag, that is set or cleard using the locomotive setup procedure. The actual direction is influenced by the flag. If switch modules are paired to the motor module the flag influences theire direction functionality too.

## Pairing
The module can be paired with a controller. In addition, up to 16 further modules can be paired, which are installed together in one locomotive. Switch modules for light are a prerequisite for using the locomotive setup function.

* Pairing with a controller can be done anytíme and is described in the controller readme.
* The paring of locomotive internal modules must be done with no other vehicle connected to the CAN line and only needs to be performed once, as long as nothing is changed in the locomotive.

The locomotive setup process is initiated by setting the main switch to off, pushing and holding the horn button and switching on the mains again.

The status led will alternately blink orange and green, if the controller is not paired, and green, if a pairing is found. Push and hold the horn button for more than 10 seconds, until the led light starts to rapidly flash orange. The motor module scans all incoming module heartbeat messages and pair to all modules offering this functionality. The process will last for about a minute, then three green blinks of the status led indicates the successfull end.

The setup mode is ended by switching the main switch to off.