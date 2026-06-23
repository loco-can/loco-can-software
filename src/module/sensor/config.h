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
 * sensor board settings
 */
#pragma once

#ifndef MODULE_SENSOR_CONFIG_H
#define MODULE_SENSOR_CONFIG_H


/* ******************************************
 * The module version is set in the config.h file
 */

#if SENSOR_MODULE_VERSION == V_2_0 || SENSOR_MODULE_VERSION == V_2_1

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8
 	
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
	#define SENSOR_MAINS_PORT 12
	#define SENSOR_DIR_PORT 13
	#define SENSOR_HORN_PORT 14
	// #define SENSOR_HORN2_PORT 9
	#define SENSOR_LIGHT_PORT 38
	// #define SENSOR_LIGHT2_PORT A2

	#define ANALOGSWITCH_MAX_POS 8

	// FUNCTION SWITCH
	// #define SENSOR_INSTRUMENT_LIGHT_PORT 4

	// FUNCTION GAUGE
	// #define SENSOR_BATTERY_VOLTAGE_PORT 6
	// #define SENSOR_MOTOR_VOLTAGE_PORT 7
	// #define SENSOR_CURRENT_PORT 8

	#define SENSOR_DRIVE_MODE SINGLE
	#define SENSOR_DRIVE_PORT A0
	// #define SENSOR_BREAK_PORT A1
	// #define SENSOR_POWER_PORT A2

#else
	#error "Set SENSOR_MODULE_VERSION in hardware.h (V_2_0 or V_2_1)"

#endif

/*
 * INCLUDE CLASS
 */
#include "main.h"

#endif
