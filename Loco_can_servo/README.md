## LOCO-Servo

The module read values of CAN messages and set the motors. The four servos can be configured to react on different messages.

The min and max positions as well as a mapping of the message values provides a wide range of usecases like drive or break setting or measure values.

When using a status message, up to 8 bits can be assigned to discrete servo motor positions.

### V2.0/V2.1a-s
The versions only differ in the pinout of the FDTI-interface.

### FTDI Interface
| V2.0     | V2.1x-s   |
|:---------|:----------|
| 1 DTS    | **1 DTS** |
| 2 **TX** | **2 RX**  |
| 3 **RX** | **3 TX**  |
| 4 3V3    | **4 3V3** |
| 5 GND    | **5 GND** |
