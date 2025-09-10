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
 * module definition and
 * controller board settings
 */

#pragma once


/* ******************************************
 * The module version is set in the config.h file
 */

/*
 * CONTROLLER ADAPTER with universal module V2.0
 * (has wrong ICP pinout)
 */
#if MODULE_VERSION == V_2_0

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_PLATFORM_ATMEGA
	#define CONTROLLER_ANALOG_RESOLUTION 1024

	#define CAN_RX 10
	#define CAN_TX 2
	#define CAN_STATUS_LED 0


	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================
	#include "../function/controller/main.h"	// pot input
	#include "../function/switch/main.h"		// switch output
	#include "../function/gauge/main.h"			// pwm and servo gauges 

	// FUNCTION PARAMETERS
	// switches
	#define CONTROLLER_MAINS_PORT A4
	#define CONTROLLER_DIR_PORT A3
	#define CONTROLLER_HORN_PORT 5
	#define CONTROLLER_HORN2_PORT 9
	#define CONTROLLER_LIGHT_PORT A5
	#define CONTROLLER_LIGHT2_PORT A2
	#define CONTROLLER_INSTRUMENT_LIGHT_PORT 4

	#define CONTROLLER_BATTERY_VOLTAGE_PORT 6
	#define CONTROLLER_MOTOR_VOLTAGE_PORT 7
	#define CONTROLLER_CURRENT_PORT 8

	#define CONTROLLER_DRIVE_MODE SINGLE
	#define CONTROLLER_DRIVE_PORT A0
	#define CONTROLLER_BREAK_PORT A1
	#define CONTROLLER_POWER_PORT A2

	// status display method
	#define CONTROLER_STATUS_MODE LED

	#if CONTROLLER_STATUS_MODE == LED
		#define CONTROLLER_STATUS_RED_PORT 2
		#define CONTROLLER_STATUS_GREEN_PORT 3
	#elif CONTROLLER_STATUS_MODE == OLED
		// supported OLED displays
	#endif


/*
 * CONTROLLER ADAPTER with universal module V2.1
 */
 #elif MODULE_VERSION == V_2_1

	// ======================================
	// BASIC SETTINGS
	// ======================================

	#define MODULE_PLATFORM_ATMEGA
	#define CONTROLLER_ANALOG_RESOLUTION 1024

	#define CAN_RX 10
	#define CAN_TX 2
	#define CAN_STATUS_LED 0

	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================
	#include "../function/controller/main.h"	// pot input
	#include "../function/switch/main.h"		// switch output
	#include "../function/gauge/main.h"			// pwm and servo gauges 

	/* define corresponding parameters */ 
	// switches
	#define CONTROLLER_MAINS_PORT A4
	#define CONTROLLER_DIR_PORT A3
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

	// status display method
	#define CONTROLER_STATUS_MODE LED

	#if CONTROLLER_STATUS_MODE == LED
		#define CONTROLLER_STATUS_RED_PORT 2
		#define CONTROLLER_STATUS_GREEN_PORT 3
	#elif CONTROLLER_STATUS_MODE == OLED
		// supported OLED displays
	#endif


/*
 * ESP32S3 based version
 * not yet implemented, only for debugging use
 */
#elif MODULE_VERSION == V_3_0

	#define MODULE_PLATFORM_ESP32
	#define CONTROLLER_ANALOG_RESOLUTION 2048

	#include "../function/controller/main.h"	// pot input
	#include "../function/switch/main.h"		// switch output
	#include "../function/gauge/main.h"			// pwm and servo gauges 

	// CAN ports
	#define CAN_RX 17
	#define CAN_TX 18
	#define CAN_STATUS_LED 46

	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	// CAN LED

#endif
