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

### core

The core directory contains all basic functions of all modules like the
CAN, serial or USB communication and the protocol definitions.

### function

A function is a single application for a certain functionality. A set
of functions in combination with teh port mapping defines a module.

### modele

In the module directory the module definitions are stored. A module is
created by a configuration file whrer the used functions, the necessary
parameters and the hardware mapping is set.

	#ifdef MODULE = {MODULE_X}
		#include "src/module/FUNCTION_1/main.h"
		#include "src/module/FUNCTION_2/main.h"
		...

		#define MODULE_VERSION {USED_BORD}
	#endif
