/*
 * Loco-CAN
 * 
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 * Version 2.x
 */

 /* *************************************************
 * module definition and
 * controller board settings
 */
#pragma once

#ifndef MODULE_CONTROLLER_CONFIG_H
#define MODULE_CONTROLLER_CONFIG_H


/* ******************************************
 * The module version is set in the hardware.h file
 */

// #pragma message "module controller config.h"

#if CONTROLLER_MODULE_VERSION == V_1_0

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_RX 10
	#define CAN_TX 2
	#define CAN_STATUS_LED 5
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8

	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	// FUNCTION PARAMETERS
	#define CONTROLLER_MAINS_PORT A1
	#define CONTROLLER_DIR_PORT A2
	// #define CONTROLLER_HORN_PORT 5
	// #define CONTROLLER_HORN2_PORT 9
	#define CONTROLLER_LIGHT_PORT A3
	// #define CONTROLLER_LIGHT2_PORT A2

	#define ANALOGSWITCH_MAX_POS 8

	// status display method
	#define CONTROLLER_STATUS_MODE LED

	#if CONTROLLER_STATUS_MODE == LED
		#define CONTROLLER_STATUS_RED_PORT 6
		#define CONTROLLER_STATUS_GREEN_PORT 7
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


/*
 * CONTROLLER ADAPTER with universal module V2.0
 * (has wrong ICP pinout)
 */
#elif CONTROLLER_MODULE_VERSION == V_2_0

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_RX 10
	#define CAN_TX 2
	#define CAN_STATUS_LED 5
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8

	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	// FUNCTION PARAMETERS
	#define CONTROLLER_MAINS_PORT A4
	#define CONTROLLER_DIR_PORT A3
	#define CONTROLLER_HORN_PORT 5
	#define CONTROLLER_HORN2_PORT 9
	#define CONTROLLER_LIGHT_PORT A5
	#define CONTROLLER_LIGHT2_PORT A2

	#define ANALOGSWITCH_MAX_POS 8

	// status display method
	#define CONTROLLER_STATUS_MODE LED

	#if CONTROLLER_STATUS_MODE == LED
		#define CONTROLLER_STATUS_RED_PORT 2
		#define CONTROLLER_STATUS_GREEN_PORT 3
	#elif CONTROLLER_STATUS_MODE == OLED
		// supported OLED displays
	#endif

	// FUNCTION SWITCH
	#define CONTROLLER_INSTRUMENT_LIGHT_PORT 4

	// FUNCTION GAUGE
	#define CONTROLLER_BATTERY_VOLTAGE_PORT 6
	#define CONTROLLER_MOTOR_VOLTAGE_PORT 7
	#define CONTROLLER_CURRENT_PORT 8

	#define CONTROLLER_DRIVE_MODE SINGLE
	#define CONTROLLER_DRIVE_PORT A0
	#define CONTROLLER_BREAK_PORT A1
	#define CONTROLLER_POWER_PORT A2


/*
 * CONTROLLER ADAPTER with universal module V2.1
 */
 #elif CONTROLLER_MODULE_VERSION == V_2_1

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_RX 10
	#define CAN_TX 2
	#define CAN_STATUS_LED 5
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8


	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	/* define corresponding parameters */ 
	// switches
	#define CONTROLLER_MAINS_PORT A4
	#define CONTROLLER_DIR_PORT A3
	#define CONTROLLER_HORN_PORT 5
	#define CONTROLLER_HORN2_PORT 9
	#define CONTROLLER_LIGHT_PORT A5
	#define CONTROLLER_LIGHT2_PORT A2

	#define ANALOGSWITCH_MAX_POS 8

	#define CONTROLLER_DRIVE_MODE SINGLE
	#define CONTROLLER_DRIVE_PORT A0
	#define CONTROLLER_BREAK_PORT A1
	// #define CONTROLLER_POWER_PORT A2

	// status display method
	#define CONTROLLER_STATUS_MODE LED

	#if CONTROLLER_STATUS_MODE == LED
		#define CONTROLLER_STATUS_RED_PORT 2
		#define CONTROLLER_STATUS_GREEN_PORT 3
	#elif CONTROLLER_STATUS_MODE == OLED
		// supported OLED displays
	#endif

	// FUNCTION SWITCH
	#define CONTROLLER_INSTRUMENT_LIGHT_PORT 4

	// FUNCTION GAUGE
	#define CONTROLLER_BATTERY_VOLTAGE_PORT 6
	// #define CONTROLLER_MOTOR_VOLTAGE_PORT 7
	#define CONTROLLER_CURRENT_PORT 8


/*
 * ESP32S3 based version
 * not yet implemented, only for debugging use
 */
#elif CONTROLLER_MODULE_VERSION == V_3_0

	#define MODULE_ARCH_ESP32

	#define PLATFORM_ANALOG_RESOLUTION 2048
	#define ANALOGSWITCH_MAX_POS 8

	// CAN ports
	#define CAN_RX 17
	#define CAN_TX 18
	#define CAN_STATUS_LED 46
	#define CAN_MAX_FILTER 16
	#define CAN_BUFFER_SIZE 16


	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	// CAN LED

#else
	#error "Set CONTROLLER_MODULE_VERSION in hardware.h (V_1_0, V_2_0, V_2_1, or V_3_0)"

#endif

/*
 * INCLUDE CLASS
 */
#include "main.h"

#endif
