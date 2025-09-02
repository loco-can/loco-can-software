/*
 * Loco-CAN
 * 
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 * Version 3.x
 */

 /* *************************************************
 * module definition
 * controller board settings
 */

#pragma once

#include "../../config.h"


/* ******************************************
 * The board version is set in the config.h file
 */


#if MODULE_VERSION == V_2_0

	#define MODULE_PLATFORM_ATMEGA

	/* include functions */
	#include "../function/controller/main.h"


	/* define corresponding parameters */ 
	// switches
	#define CONTROLLER_MAINS_PORT  
	#define CONTROLLER_DIR_PORT 
	#define CONTROLLER_HORN_PORT 5
	#define CONTROLLER_HORN2_PORT 9
	#define CONTROLLER_LIGHT_PORT A5
	#define CONTROLLER_LIGHT2_PORT A2
	#define CONTROLLER_INSTRUMENT_LIGHT_PORT 4

	#define CONTROLLER_BATTERY_VOLTAGE_PORT 6
	// #define CONTROLLER_MOTOR_VOLTAGE_PORT 7
	#define CONTROLLER_CURRENT_PORT 8

	#define CONTROLLER_DRIVE_MODE SINGLE
	#define CONTROLLER_DRIVE_PORT A0
	#define CONTROLLER_BREAK_PORT A1
	// #define CONTROLLER_POWER_PORT A2

	// CAN LED
	#define CAN_STATUS_LED 0

	// status display method
	#define CONTROLER_STATUS_MODE LED

	#if CONTROLLER_STATUS_MODE == LED
		#define CONTROLLER_STATUS_RED_PORT 2
		#define CONTROLLER_STATUS_GREEN_PORT 3
	#elif CONTROLLER_STATUS_MODE == OLED
		// supported OLED displays
	#endif


#elif MODULE_VERSION == V_2_1

	#define MODULE_PLATFORM_ATMEGA
	// #include "src/function/controller/main.h"


#elif MODULE_VERSION == V_3_0

	#define MODULE_PLATFORM_ESP32
	// #include "src/function/controller/main.h"

#endif
