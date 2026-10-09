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
 * The module version is set in the hardware.h file.
 * Skip the pin map when another module is selected so those
 * boards keep their own CAN_STATUS_LED and filter sizes.
 */

#if defined(ELECTRIC_MODULE_VERSION) && ELECTRIC_MODULE_VERSION == V_2_0

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_SS 17
	#define CAN_INT 18
	#define CAN_STATUS_LED 9
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

#elif defined(ELECTRIC_MODULE_VERSION) && ELECTRIC_MODULE_VERSION == V_2_1

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_SS 17
	#define CAN_INT 18
	#define CAN_STATUS_LED 9
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

#elif defined(ELECTRIC_MODULE_VERSION)
	#error "No valid board version selected"
#endif

#if defined(ELECTRIC_MODULE_VERSION)
/*
 * Battery voltage inputs. The EEPROM battery count selects how many of
 * these are measured and published, up to ELECTRIC_BATT_MAX.
 */
#define ELECTRIC_BATT_0 A0
#define ELECTRIC_BATT_1 A2
#define ELECTRIC_BATT_2 A6

/*
 * Optional relay in the motor power line. The direct plugin opens it
 * before a direction change. Comment the define out when it is not fitted.
 */
#define DRIVE_POWER 6

/*
 * INCLUDE CLASS
 */
#include "main.h"
#endif

#endif
