# The CAN protocol

The CAN messages are identified by identifier. Loco-CAN uses its own ID-system. Every module can send messages and register to ids that are interesting. There are some spezial messages, that have a spezial impact on the overall function. Primary it is the communication between controllers and the motor module.

Loco-CAN uses the extended 29 bit identifiers. The 11 bit standard id contains the message type, 16 bits of the extended id holds an unique hardware id of the module. This uuid is used for the setup of module parameters.

## Heartbeat
Heartbeat messages are used to identify the presents of modules. There are different heartbeats:

* Module heartbeat 0x700: Each module sends an identification message containing three bytes (count in bytes, software version, module type id). If the main switch flag is false, the module heartbeat is sent every 10 seconds, if it is true, the message is sent every second
* Controller heartbeat 0x600: Only the active controller sends the heartbeat, if the main switch is on
* End light hearbeat 0x610: A special message, that is used to check the train integrity

## Heartbeat mode
A heartbeat mode can be implemented in every module. In the case, the heartbeat is missing, certain actions can take place, like stopping a motor or setting a servo to a certain position.

## Heartbeat and the Controller priority
There are no limitations of controllers connected to the bus, but only one can be active. If the controller can get active, depends on the following conditions:

### No heartbeat is received
* the controller is activated => telemetry data is used from a paired driving module or the first found module. All driving modules are controlled.

### A controller heartbeat is received:
* The controller is paired with a driving module and the paired module is present => the controller is activated for the paired driving module and all modules not paired or without a fitting controller
* The controller is paired but the module is not present => the controller keeps inactive (error status displayed)

A missing heartbeat during operation can lead to an emergency stop, depending on the configuration.

* Only on controller is active => The train is stopped imediately

### More than one controller is active
* The heartbeat fail relates to an unpaired => The train is stopped imediately
* The heartbeat fails from a paired controller => The driving module switch to the first found controller

## End light heartbeat
All modules with a hearbeat mode can register the end light message. If no end light is found, the missing heartbeat is ignored. When connecting an end light, all modules, that have registered the hearbeat set a end light present flag to true. In this case a failing hearbeat results in an imediate stop of the train.

## Module pairing
In some cases, it makes sense to combine modules. With a pairing the uuid of the involved modules are stored in euch module. Some functions can depend on the state of paired units.

### Motor-Switch Pairing 
If a switch module is used in a locomotive with headlight on both ends of the vihicle, it has to be paired to the motor module. It is a requirement for use of the locomotive setup to be able to switch the lamps depending on the direction.

### Controler-Motor Pairing
Basically it is not necessary to pair a controller to a motor module. If one wants to drive in multiple traction, measured values of a certain locomotive can be displayed by pairing on the driver's desk. Otherwise the values of the first locomotive found are displayed. If there are several locomotive controller pairs in one train, the respective locomotives can be operated independently of each other by several locomotive drivers.


## Drive Module Status Messages
The message is sent as part of the module heartbeat, when the main switch is off and as status message, when the main switch is on.

Byte 0

|Bit 7|Bit 6|Bit 5|Bit 4 |Bit 3  |Bit 2|Bit 1|Bit 0|
|-----|-----|-----|------|-------|-----|-----|-----|
|error|ready|stop |paired|reverse|dir  |drive|mains|

* error: the module reports an error
* ready: ready to drive (mains on, direction selected)
* stop: the locomotive is standing
* reverse: the direction works the other way around 
* paired: the drive module is paired with a controller, the uuid of the paired controller is stored in the module
* dir: the selected direction (0: forware, 1: reward)
* mains: the main switch status (0: off, 1: on)

# Operation Messages

## High priority messages

|Name|Value|
|----|-----|
|CAN_ID_CURRENT|0x100|
|CAN_ID_MOTOR_CURRENT|0x110|
|CAN_ID_BATT_CURRENT|0x120|
|CAN_ID_LIGHT_CURRENT|0x130|

## Mid priority messages

|Name|Value|
|----|-----|
|CAN_ID_SPEED|0x200|
|CAN_ID_DIR|0x210|
|CAN_ID_SIGNAL|0x220|
|CAN_ID_TACHO|0x230|

## Low priority messages

|Name|Value|
|----|-----|
|CAN_ID_VOLTAGE|0x300|
|CAN_ID_MOTOR_VOLTAGE|0x310|
|CAN_ID_BATT_VOLTAGE|0x320|
|CAN_ID_BATT_1_VOLTAGE|0x321|
|CAN_ID_BATT_2_VOLTAGE|0x322|

## Command messages

|Name|Value|
|----|-----|
|CAN_ID_DRIVE|0x400|
|CAN_ID_LIGHT|0x410|
|CAN_ID_SWITCH|0x420|

## Status messages

|Name|Value|
|----|-----|
|CAN_ID_STATUS|0x500|

## Heardbeat values
The heartbeat is sent from the active controller and used to monitor the connection between controller and motor modules. The train end latern signals are registered in the controller when starting to drive. A change in the latern signal list while driving leads to a emergency stop (switch off the heartbeat signal)

|Name|Value|
|----|-----|
|CAN_ID_HEARTBEAT|0x600|
|CAN_ID_TRAINEND|0x610|

sent from train end laterns

## Setup

The setup command is used to send and receive module setup data.
* 0x7FF request setup info from module (0 byte message length)
        the identifier uuid is ignored and the info is sent any way
* 0x7nn returns info packages (nn = data id)
        + 8 bytes text description
        adding the own uuid in the extended identifier
        so no other module can interpret it as a write command

* write commands are sent with the target uuid in the first two bytes
0x600   set module name

0x6nn   set data
        nn => data id + max 8 bytes of data

* message  bytes 0, 1 	=> UUID of module
           byte 2		=> message id
* 0xFF = module name
           bytes 3-7 	=> data

## Masks

|Name|Value|
|----|-----|
|CAN_ID_MASK|0x770|
|CAN_REQUEST_MASK|0x7FF|
|CAN_ID_REQUEST|0x7FF|
|CAN_REPLY_MASK|0x7FF|
|CAN_ID_REPLY|0x780|
|CAN_SETUP_MASK|0x700|
|CAN_ID_SETUP|0x700|

|N