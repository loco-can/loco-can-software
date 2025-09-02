# LOCO-CAN Software

The hardware schematics and PCB layouts are managed in the [LOCO-CAN Hardware](https://github.com/loco-can/loco-can-hardware) repository.

The system is still under developement.

The LOCO-CAN-system is designed for the use is miniature railway vehicles. It is based on the CAN-bus to connect the different modules inside a locomotive, but also between vehicles in a train.

The CAN protocol definitions can be extended to add new functionality. Board designs are available in different versions, due to the process of evolvement. Besides bugs in the board design details like the pinout of the program interface differ between the versions and the electric spezifications of the CAN-connector. The communication protocol is compatible betweet all versions.


## Software design
Each module code basically is designed using the same base structure.

	module-name.ino    // main part with can initialisation
	
	config.h           // module configuration
	module_board.h     // hardware configuration

	module.cpp         // module dependent main part
	module.h

	README.md          // module description+

The module-name.ino file initializes the can comunication and the module code. The module.cpp contains the main methods for the module.



## Design
The design is highly modular. For the simple control of an electric locomotive only a control and a motor module is needed. When using the latest motor module version, values for motor and battery voltage is sent to the can bus. The control module has the ability to connect meters to display this values.

The structure of a bus system makes it easy to expand the contruction with additional functions. New modules, i.e. for light, controlling servos or measuring current, only need to be hooked into the CAN-line.


# Configuration
**This functionality is not yet implemented**
The modules are shipped with a standard configuration, like the assignment of the switch module outputs to light stati or the potentiometer value for neutral position of the throttle. The paramters can be adjusted using a USB connection or the webinterface of a WiFi-modul. Connecting with a laptop or smartphone gives access to a user-friendly interface for this task.

## Software Updates
The pinout of the FTDI interface for flashing new software differs in the early versions. The standard pinout will be implemented in all revisions, divergent pinouts are listed with the version descriptions.


## CAN Software Updates
**This functionality should be implemented in the future**
A future update should bring the ability for a software update using the CAN-bus instead of the serial port. The update process is initialized and controlled by a WiFi-module. Using the webinterface the new firmware for a module type is loaded in the WiFi-module.

All connected modules with this type get a command to enter the CAN-bootloader. The update command sets the update bit in the CAN-bootloader as well as the count of received and saved packages in the EEPROM. If the update process is terminated, it can be restarted at the last package that was received.

The CAN-bootloader communicates with the sending WiFi module using a handshake for each package. The WiFi module waits for the reply of all modules that are in the update process for the next package to be sent.

When finished all updated modules reset the update bit and restart automatically.


# Modules
The modules are intendet for a special use, like switching light or controlling a motor. All modules are equipped with two CAN connectors. A detailed description is found in the README.md file in the corresponding subdirectories.

Beside the spezialized modules universial versions with a general I/O connectors can be used for different tasks. Two versions with compatible extension connectors are available, one with wireless capability, witch is based on the more powerfull 32 bit ESP32 processor.


# Libraries for the Loco-CAN Project

All libraries, that are used by the Loco-CAN project are collected in this repository to guaratee the make process and all functions. Some libraries are used from other projects.

* AnalogSwitch
* CAN_Com
* CAN_Com_Settings
* Flags
* IntelliLED
* LocoCAN
* RampPWM
* IntelliTimeout

## Other Libraries

* ArduinoUniqueID   (Luiz Henrique Cassettari <ricaun@gmail.com>)
* CAN-0.3.1         (Sandeep Mistry <sandeep.mistry@gmail.com>)
* RokkitHash        (This is a port of Paul Hsieh's "SuperFastHash" (http://www.azillionmonkeys.com/qed/hash.html))
