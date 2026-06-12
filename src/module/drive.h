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

#if MODULE_VERSION == V_1_0

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_PLATFORM_ESP32
	#define PLATFORM_ANALOG_RESOLUTION 1024

	#define CAN_RX 17
	#define CAN_TX 18
	#define CAN_STATUS_LED 46
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8

	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	// DRIVE FUNCTION PARAMETERS
	// H-BRIDGE CONTROL
	#define DRIVE_PWM 15
	#define DRIVE_FORW 21
	#define DRIVE_REV 2
	#define DRIVE_BREAK 3

	// SENSORS
	#define DRIVE_MOTOR_VOLTAGE_PLUS 4
	#define DRIVE_MOTOR_VOLTAGE_MINUS 5
	#define DRIVE_CURRENT 6

	// TEMPERATUR SENSOR
	#define DRIVE_TEMP_SDA 7
	#define DRIVE_TEMP_SCK 8

	// ======================================
	// CONTROL FUNCTION PARAMETERS
	#define CONTROLLER_MAINS_PORT 12
	#define CONTROLLER_DIR_PORT 13
	#define CONTROLLER_HORN_PORT 14
	// #define CONTROLLER_HORN2_PORT 9
	#define CONTROLLER_LIGHT_PORT 38
	// #define CONTROLLER_LIGHT2_PORT A2

	#define ANALOGSWITCH_MAX_POS 8

	// status display method
	#define CONTROLER_STATUS_MODE LED

	#if CONTROLLER_STATUS_MODE == LED
		#define CONTROLLER_STATUS_RED_PORT 41
		#define CONTROLLER_STATUS_GREEN_PORT 42
	#elif CONTROLLER_STATUS_MODE == OLED
		// supported OLED displays
	#endif

	// FUNCTION SWITCH
	// #define CONTROLLER_INSTRUMENT_LIGHT_PORT 4

	// FUNCTION GAUGE
	// #define CONTROLLER_BATTERY_VOLTAGE_PORT 6
	// #define CONTROLLER_MOTOR_VOLTAGE_PORT 7
	// #define CONTROLLER_CURRENT_PORT 8

	#define CONTROLLER_DRIVE_MODE SINGLE
	#define CONTROLLER_DRIVE_PORT A0
	// #define CONTROLLER_BREAK_PORT A1
	// #define CONTROLLER_POWER_PORT A2

	#include "../function/drive/main.h"	// pot input


#endif
