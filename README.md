# Loco-CAN

@author: Thomas H Winkler
@copyright: 2025
@lizence: GG0

There is only of software package for all modules of the Loco-CAN system.
Before compiling the targe module must be set in the config.h file.

## File Structure

The directory structure of the project:

	Loco-CAN
	- doc
	- src
		- core
		- function
		- module
	LocoCANcore.cpp
	LocoCANcore.h

### doc

In this early stage of developement the doc directory contains different text files or old
code, that may be helpfull in creating the new project from the old classes. In the end in the
directory only the documentation will remain.

### core

The core directory contains all basic functions that are used by all modules like the
CAN, serial or USB communication, basic classes and the protocol definitions.

	analogSwitch	creates a stepping switch on a single analog input
	button			a pushbutton detecting multiple touches and long touches
	can 			the can communication
	hash			create a hash
	led 			a single or double color led with on/off blink and flash capability
	pwm				a pwm with ramps
	timeout			timeout with retrigger
	uniqueID		gets 16 bit unique ids from the mcu
	value			handle analogue values with precission and a percentage value 

### function

A function is a single application for a certain functionality. A set
of functions in combination with the port mapping defines a module.

	controller		locomotive controller with switches, pots, leds and gauges

### modele

In the module directory the hardware modules are defined. A module is
created by a configuration file where the used platform, the functions, the necessary
parameters and the hardware mapping is set.

	#ifdef MODULE_VERSION = {MODULE_X}

		// ======================================
		// BASIC SETTINGS
		// ======================================

		#define MODULE_PLATFORM_xxx

		#define CAN_RX xx
		#define CAN_TX xx
		#define CAN_STATUS_LED xx

		// ======================================
		// INCLUDED FUNCTIONS
		// ======================================
		#include ".../function/FUNCTION_NAME/main.h"

		/* define corresponding parameters */ 
		...

		#define MODULE_VERSION {USED_BORD}
	#endif
