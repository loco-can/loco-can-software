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

#ifndef CONFIG_H
#define CONFIG_H


/* ========================================================================
 * GLOBAL SETTINGS
 * uncommend to activate serial debug */
#define DEBUG

/* uncommend to get additional information on serial */
#define DEVEL


/* CAN COMMUNICATION */
#define CAN_SPEED 500

/* time to detect a long pressed button */
#define BUTTON_LONG_PUSH_TIME 1000


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
PROGRAMMER MODULE
	The programmer module is the interface to program the Loco-CAN system.

Versions:
	V_2_0_0
	V_3_0_0
======================================================================== */
#define PROGRAMMER_MODULE "src/module/programmer/config.h"


/* ========================================================================
UNIVERSAL MODULE
	The universal module has no special hardware. It has GPIO ports, that
	can be used for different functions. There is no specific module but
	all module software can be used as hardware setting.

Versions:
	V_2_0_0
	V_2_1_0
======================================================================== */


/* ========================================================================
CONTROLLER MODULE
	The controller module is the interface to control locomotives. Pots,
	switches, status lights, OLED displays and gauges are connected to it.

Versions:
	V_2_0_0
	V_2_1_0
	V_3_0_0
======================================================================== */
#define CONTROLLER_MODULE "src/module/controller/config.h"


/* ========================================================================
VEHICLE MODULE
	The vehicle module is the interface to control an external power driver
	to the Loco-CAN system.

Versions:
	V_2_0_0
	V_2_1_0
	V_2_2_0
======================================================================== */
#define VEHICLE_MODULE "src/module/vehicle/config.h"


/* ========================================================================
SWITCH MODULE
	The switch module offers six outputs capable of switching up to 5 Amps
	and 30 Volts per pin.

Versions:
	V_2_0_0
	V_2_1_0
======================================================================== */
#define SWITCH_MODULE "src/module/switch/config.h"


/* ========================================================================
SENSOR MODULE
	The sensor module has a 50 Amps current sensor, four voltage inputs and
	a pulse input.

Versions:
	V_2_0_0
	V_2_1_0
======================================================================== */
#define SENSOR_MODULE "src/module/sensor/config.h"


/* ========================================================================
SERVO MODULE
	Up to four RC model servo motors can be controlled by this module.

Versions:
	V_2_0_0
	V_2_1_0
======================================================================== */
#define SERVO_MODULE "src/module/servo/config.h"


/* ========================================================================
WIFI MODULE
	ESP32 wireless CAN bridge. Two modules pair over ESP-NOW and copy
	CAN frames between isolated bus segments when no physical cable
	is connected.

Versions:
	V_2_0_0
	V_3_0_0
======================================================================== */
#define WIFI_MODULE "src/module/wifi/config.h"


/* ========================================================================
LED MODULE
	A module with LED drivers.

Versions:
	V_2_0_0
======================================================================== */
//#define LED_MODULE "src/module/led/config.h"


/* ========================================================================
DRIVE MODULE
	A 4Q motor driver.

Versions:
	V_3_0_0
	V_3_1_0
======================================================================== */
//#define DRIVE_MODULE "src/module/drive/config.h"


/* ===================================================================== */
// VERSIONS
#include "can_protocol.h"

// major.minor.patch, for example V_2_1_12 is firmware 2.1.12.
// A new patch keeps the hardware block of the same major.minor.
#define V_1_0_0 LOCO_VERSION(1, 0, 0)
#define V_2_0_0 LOCO_VERSION(2, 0, 0)
#define V_2_1_0 LOCO_VERSION(2, 1, 0)
#define V_2_2_0 LOCO_VERSION(2, 2, 0)
#define V_3_0_0 LOCO_VERSION(3, 0, 0)
#define V_3_1_0 LOCO_VERSION(3, 1, 0)


/* ===================================================================== */
//
// select module and version
//
/* ===================================================================== */
#include "../hardware.h"

/* ====================================================================== */

#endif