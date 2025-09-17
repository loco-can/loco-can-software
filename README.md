# Loco-CAN Software

@author: Thomas H Winkler
@copyright: 2020-2025
@lizence: GG0

Loco-CAN is a bus system for passenger garden railways. It is based on the CAN bus, which was developed in the 1980s for cars to reduce wiring costs. Communication on the two-wire line is very robust and insensitive to interference. The Loco-CAN system is very modular, simplifies the electrical setup in garden railway vehicles, and offers numerous functions.

## File Struktur

The software for all possible modules is contained in a single project. The selection of which specific variant is to be compiled is made via a configuration file, in which the hardware version used can also be specified. The basic functions are supported by all hardware versions from 1.0 onwards; only a USB port or a WiFi connection are reserved for the ESP32 family.

The project has the following file structure:

	- loco-can-software
		- doc
		- license
		- src
			- core
			- function
			- module
			can_protocol.h
			LocoCANcore.cpp
			LocoCANcore.h
		config.h
		loco-can-software.ino
		README.md

### doc

The doc directory contains all necessary documentation and examples as prototype to create new functions and modules.

### license

The full GNU GENERAL PUBLIC LICENSE Version 3 license text from 29 June 2007.

### src

The src directory contains all of the project's code. It is divided into three subdirectories:

	- core
	- function
	- module

The .ino file creates the LocoCANcore, which is located here. The core class creates the software runtime environment, integrates all necessary functions, and starts them.

#### core

The core directory contains all global methods that can be used for the modules. CAN communication is central. The other methods serve to standardize the control of hardware elements and to monitor program execution. The following methods are available:

	analogSwitch
	button
	can
	hash
	led
	measure
	pwm
	servo
	timeout
	uniqueID
	value
	watchdog

#### function

Functions are complex methods that perform a specific sequence of steps in the Loco-CAN system. This can be a controller to which potentiometers and switches are connected and which can be used to control a locomotive, an interface for correctly and safely controlling a motor stage, or the function of switching switches with defined CAN messages.

A module can use one function, but it is also possible to combine several functions on a single piece of hardware. In this case, communication between the functions takes place via normal CAN messages within the software before they are forwarded to the external bus.

Functions can be easily expanded. The doc directory contains examples that describe the basic structure and can serve as templates.

A function must be entered in the following places in order to be used:

**src/LocoCANcore.h**

	#ifdef {FUNCTION_NAME_H}
		{FUNCTION_NAME} _{function_name};
	#endif

**src/LocoCANcore.cpp**

	void LocoCANcore::begin(void) {

		...

		// =========================
		// start function {FUNCTION_NAME}
		#ifdef {FUNCTION_NAME_H}
			_{function_name}.begin(func++);
		#endif

		...
	}

	void LocoCANcore::update(void) { 

		...

		// ==========================
		// update function {FUNCTION_NAME}
		#ifdef {FUNCTION_NAME_H}
			_{function_name}.update(can_message);
		#endif
	
		...
	}

#### module

A module represents hardware with certain parameters, such as the architecture used, the inputs and outputs, and specialized electronic components. It is defined in an h-file. The selection of which module file is used is made at the end of the config.h file. The module name and module version are specified there.

There is a definition block for each hardware version, which may differ in terms of the scope of functions.

	#pragma once

	/* ******************************************
	 * The module version is set in the config.h file
	 */

	#ifdef MODULE_VERSION = {module_version}

		// ======================================
		// BASIC SETTINGS
		// ======================================

		#define MODULE_PLATFORM_ATMEGA
		#define PLATFORM_ANALOG_RESOLUTION 1024

		#define CAN_RX 10
		#define CAN_TX 2
		#define CAN_STATUS_LED 5
		#define CAN_MAX_FILTER 8
		#define CAN_BUFFER_SIZE 8

		// ======================================
		// INCLUDED FUNCTIONS
		// ======================================

		/* define corresponding parameters */ 

		#include ".../function/{function_name}/main.h"
		...

	#endif

The BASIC SETTINGS block must be present. It is used to configure the basic settings for the core methods. The INCLUDED FUNCTIONS block contains all functions that are used in the module. The necessary parameters must follow for each function.

A module is defined in the config.h header file.

**config.h**

	#define {module_name} "{path to module header file}"
