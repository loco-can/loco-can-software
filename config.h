/*
 * LOCO-DRIVE module configuration file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 *
 * Version 3.0
 */


#pragma once


/*
	In the config.h all supported modules are listed with the module include
	file and the available versions. To select a module for compilation the MODULE
	and MODULE_VERSION must be set at the end of this file.

	Take care, that the correct platform is selected in the Arduino IDE. The platform
	to be used is set in the module definitions depending on the version. For simple
	selection use this platforms:
	 
		ATMEGA based boards V2.x
		ESP32 based boards V3.x
 */


/* ========================================================================
UNIVERSAL MODULE
	The universal module has no special hardware. It has GPIO ports, that
	can be used for different functions.

Versions:
	V_2_0
	V_2_1
======================================================================== */
#define UNIVERSAL_MODULE "src/module/uinversal.h"


/* ========================================================================
CONTROLLER MODULE
	The controller module is the interface to control locomotives. Pots,
	switches, status lights, OLED displays and gauges are connected to it.

Versions:
	V_2_0
	V_2_1
	V_3_0
======================================================================== */
#define CONTROLLER_MODULE "src/module/controller.h"


/* ========================================================================
MOTOR MODULE
	The motor module is the interface to control an external power driver
	to the Loco-CAN system.

Versions:
	V_2_0
	V_2_1
======================================================================== */
#define MOTOR_MODULE "src/module/uinversal.h"


/* ========================================================================
SWITCH MODULE
	The switch module offers six outputs capable of switching up to 5 Amps
	and 30 Volts per pin.

Versions:
	V_2_0
	V_2_1
======================================================================== */
#define SWITCH_MODULE "src/module/switch.h"


/* ========================================================================
SENSOR MODULE
	The sensor module has a 50 Amps current sensor, four voltage inputs and
	a pulse input.

Versions:
	V_2_0
======================================================================== */
#define SENSOR_MODULE "src/module/sensor.h"


/* ========================================================================
SERVO MODULE
	Up to four RC model servo motors can be controlled by this module.

Versions:
	V_2_0
	V_2_1
======================================================================== */
#define SERVO_MODULE "src/module/servo.h"


/* ========================================================================
LED MODULE
	A module with LED drivers.

Versions:
	V_2_0
======================================================================== */
#define LED_MODULE "src/module/led.h"


/* ========================================================================
DRIVE MODULE
	A 4Q motor driver.

Versions:
	V_3_0
	V_3_1
======================================================================== */
#define DRIVE_MODULE "src/module/drive.h"


/* ===================================================================== */
// VERSIONS
#define V_1_0 10
#define V_2_0 20
#define V_2_1 21
#define V_3_0 30
#define V_3_1 31


/* ===================================================================== */
//
// select module and version
//
/* ===================================================================== */
#define MODULE CONTROLLER_MODULE
#define MODULE_VERSION V_2_0


#include MODULE
/* ====================================================================== */