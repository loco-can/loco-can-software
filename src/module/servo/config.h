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

#ifndef MODULE_SERVO_CONFIG_H
#define MODULE_SERVO_CONFIG_H


/* ******************************************
 * The module version is set in the config.h file
 */

#if MODULE_VERSION == V_1_0

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR
	#define PLATFORM_ANALOG_RESOLUTION 1024

	#define CAN_RX 10
	#define CAN_TX 2
	#define CAN_STATUS_LED 5
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8

	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	// FUNCTION PARAMETERS
	#define SERVO_MAINS_PORT A1
	#define SERVO_DIR_PORT A2
	// #define SERVO_HORN_PORT 5
	// #define SERVO_HORN2_PORT 9
	#define SERVO_LIGHT_PORT A3
	// #define SERVO_LIGHT2_PORT A2

	#define ANALOGSWITCH_MAX_POS 8


	// FUNCTION SWITCH
	// #define SERVO_INSTRUMENT_LIGHT_PORT 4

	// FUNCTION GAUGE
	// #define SERVO_BATTERY_VOLTAGE_PORT 6
	// #define SERVO_MOTOR_VOLTAGE_PORT 7
	// #define SERVO_CURRENT_PORT 8

	#define SERVO_DRIVE_MODE SINGLE
	#define SERVO_DRIVE_PORT A0
	// #define SERVO_BREAK_PORT A1
	// #define SERVO_POWER_PORT A2


/*
 * CONTROLLER ADAPTER with universal module V2.0
 * (has wrong ICP pinout)
 */
#elif MODULE_VERSION == V_2_0

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR
	#define PLATFORM_ANALOG_RESOLUTION 1024

	#define CAN_RX 10
	#define CAN_TX 2
	#define CAN_STATUS_LED 5
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8

	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	// FUNCTION PARAMETERS
	#define SERVO_MAINS_PORT A4
	#define SERVO_DIR_PORT A3
	#define SERVO_HORN_PORT 5
	#define SERVO_HORN2_PORT 9
	#define SERVO_LIGHT_PORT A5
	#define SERVO_LIGHT2_PORT A2

	#define ANALOGSWITCH_MAX_POS 8


	// FUNCTION SWITCH
	#define SERVO_INSTRUMENT_LIGHT_PORT 4

	// FUNCTION GAUGE
	#define SERVO_BATTERY_VOLTAGE_PORT 6
	#define SERVO_MOTOR_VOLTAGE_PORT 7
	#define SERVO_CURRENT_PORT 8

	#define SERVO_DRIVE_MODE SINGLE
	#define SERVO_DRIVE_PORT A0
	#define SERVO_BREAK_PORT A1
	#define SERVO_POWER_PORT A2


/*
 * CONTROLLER ADAPTER with universal module V2.1
 */
 #elif MODULE_VERSION == V_2_1

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR
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
	// switches
	#define SERVO_MAINS_PORT A4
	#define SERVO_DIR_PORT A3
	#define SERVO_HORN_PORT 5
	#define SERVO_HORN2_PORT 9
	#define SERVO_LIGHT_PORT A5
	#define SERVO_LIGHT2_PORT A2

	#define ANALOGSWITCH_MAX_POS 8

	#define SERVO_DRIVE_MODE SINGLE
	#define SERVO_DRIVE_PORT A0
	#define SERVO_BREAK_PORT A1
	// #define SERVO_POWER_PORT A2

	// status display method
	#define CONTROLER_STATUS_MODE LED

	#if SERVO_STATUS_MODE == LED
		#define SERVO_STATUS_RED_PORT 2
		#define SERVO_STATUS_GREEN_PORT 3
	#elif SERVO_STATUS_MODE == OLED
		// supported OLED displays
	#endif

	// FUNCTION SWITCH
	#define SERVO_INSTRUMENT_LIGHT_PORT 4

	// FUNCTION GAUGE
	#define SERVO_BATTERY_VOLTAGE_PORT 6
	// #define SERVO_MOTOR_VOLTAGE_PORT 7
	#define SERVO_CURRENT_PORT 8


/*
 * ESP32S3 based version
 * not yet implemented, only for debugging use
 */
#elif MODULE_VERSION == V_3_0

	#define MODULE_ARCH_ESP32
	#define PLATFORM_ANALOG_RESOLUTION 2048


	// CAN ports
	#define CAN_RX 17
	#define CAN_TX 18
	#define CAN_STATUS_LED 46
	#define CAN_MAX_FILTER 16
	#define CAN_BUFFER_SIZE 16


	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	#define ANALOGSWITCH_MAX_POS 8
	// CAN LED

#endif

/*
 * INCLUDE CLASS
 */
#include "main.h"

#endif