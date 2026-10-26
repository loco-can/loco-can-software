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
 * vehicle board settings
 */
#pragma once

#ifndef MODULE_VEHICLE_CONFIG_H
#define MODULE_VEHICLE_CONFIG_H

/* 8-bit type: bit7 wireless, bits6-4 group, bits3-0 type (doc/SETTINGS.md). */
#define MODULE_TYPE TYPE_VEHICLE


/* ******************************************
 * The module version is set in the hardware.h file.
 * Skip the pin map when another module is selected so those
 * boards keep their own CAN_STATUS_LED and filter sizes.
 */

#if defined(VEHICLE_MODULE_VERSION) && LOCO_VERSION_IS(VEHICLE_MODULE_VERSION, 2, 0)

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_SS 10
	// #define CAN_INT 18
	#define CAN_STATUS_LED 5
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
	#define DRIVE_MOTOR_VOLTAGE_PLUS A0
	#define DRIVE_MOTOR_VOLTAGE_MINUS A1

#elif defined(VEHICLE_MODULE_VERSION) && LOCO_VERSION_IS(VEHICLE_MODULE_VERSION, 2, 1)

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_SS 10
	// #define CAN_INT 18
	#define CAN_STATUS_LED 5
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
	#define DRIVE_MOTOR_VOLTAGE_PLUS A0
	#define DRIVE_MOTOR_VOLTAGE_MINUS A1

#elif defined(VEHICLE_MODULE_VERSION) && LOCO_VERSION_IS(VEHICLE_MODULE_VERSION, 2, 2)

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_SS 12 // pin 16	
	// #define CAN_INT 18
	#define CAN_STATUS_LED 7
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8

	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	// DRIVE FUNCTION PARAMETERS
	// CONTROLLER
	#define DRIVE_PWM 9 // pin 13
	#define DRIVE_BREAK 10 // pin 14
	#define DRIVE_FORW 5 // pin 9
	#define DRIVE_REV 6 // pin 10

	#define DRIVE_ENABLE 17 // pin 26

	#define DRIVE_SPEED_SENSOR 2 // pin 32

	// OUTPUTS
	#define DRIVE_LIGHT 3 // pin 1
	#define DRIVE_HORN 4 // pin 2

	// SENSORS
	#define DRIVE_MOTOR_VOLTAGE_PLUS A0 // pin 23
	#define DRIVE_MOTOR_VOLTAGE_MINUS A1 // pin 24
	#define DRIVE_BATTERY_VOLTAGE A2 // pin 25

#elif defined(VEHICLE_MODULE_VERSION)
	#error "No valid board version selected"
#endif

#if defined(VEHICLE_MODULE_VERSION)
/*
 * Battery voltage inputs. The EEPROM battery count selects how many of
 * these are measured and published, up to VEHICLE_BATT_MAX.
 * V2.2 has one dedicated battery ADC; extra slots reuse that pin.
 */
#if LOCO_VERSION_IS(VEHICLE_MODULE_VERSION, 2, 2)
	#define VEHICLE_BATT_0 DRIVE_BATTERY_VOLTAGE
	#define VEHICLE_BATT_1 DRIVE_BATTERY_VOLTAGE
	#define VEHICLE_BATT_2 DRIVE_BATTERY_VOLTAGE
#else
	#define VEHICLE_BATT_0 A0
	#define VEHICLE_BATT_1 A2
	#define VEHICLE_BATT_2 A6

	/*
	 * Optional relay in the motor power line. The direct plugin opens it
	 * before a direction change. V2.2 uses pin 6 as DRIVE_REV, so this
	 * contact is not fitted there.
	 */
	#define DRIVE_POWER 6
#endif

/*
 * INCLUDE CLASS
 */
#include "main.h"
#endif

#endif
