# Loco-CAN Software

@author: Thomas H Winkler
@copyright: 2020-2026
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
			- module
			can_protocol.h
			config.h
			LocoCANcore.cpp
			LocoCANcore.h
		hardware.h
		loco-can-software.ino
		README.md

### doc

The doc directory contains all necessary documentation and examples as prototype to create new functions and modules.

### license

The full GNU GENERAL PUBLIC LICENSE Version 3 license text from 29 June 2007.

### src

The src directory contains all of the project's code. It is divided into two subdirectories:

	- core
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


### modules

A module contains the specific code for the required functions for a hardware. It is called from the core and can use the core functions. The module config file contains the definitions for different hardware versions.

The module is automatically included at the end of the hardware.h file. It is started in the LocoCANcore class.

The hardware settings for a module are defined in the config.h inside of the module subdirectory. At the end of config.h the main.h header of the module class is included.

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

	#endif

	/*
	 * INCLUDE CLASS
	 */
	#include "main.h"

The BASIC SETTINGS block must be present. It is used to configure the basic settings for the core methods. The INCLUDED FUNCTIONS block contains all functions that are used in the module. The necessary parameters must follow for each function.
