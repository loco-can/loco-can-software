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
 * electric locomotive board settings
 */
#pragma once

#ifndef MODULE_ELECTRIC_CONFIG_H
#define MODULE_ELECTRIC_CONFIG_H


/* ******************************************
 * The module version is set in the hardware.h file
 */

#if ELECTRIC_MODULE_VERSION == V_2_0

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
	// CONTROLLER
	#define DRIVE_PWM 15
	#define DRIVE_BREAK 3
	#define DRIVE_FORW 21
	#define DRIVE_REV 2

	// SENSORS
	#define DRIVE_MOTOR_VOLTAGE_PLUS 4
	#define DRIVE_MOTOR_VOLTAGE_MINUS 5

#elif ELECTRIC_MODULE_VERSION == V_2_1

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
	// CONTROLLER
	#define DRIVE_PWM 15
	#define DRIVE_BREAK 3
	#define DRIVE_FORW 21
	#define DRIVE_REV 2

	// SENSORS
	#define DRIVE_MOTOR_VOLTAGE_PLUS 4
	#define DRIVE_MOTOR_VOLTAGE_MINUS 5

#else
	#error "No valid board version selected"
#endif

/*
 * INCLUDE CLASS
 */
#include "main.h"

#endif
