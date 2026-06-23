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

#if SERVO_MODULE_VERSION == V_2_0 || SERVO_MODULE_VERSION == V_2_1

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
	// servo output
	#define SERVO_1 2
	#define SERVO_2 3
	#define SERVO_3 4
	#define SERVO_4 5

	// pulse input
	#define PULSE A6

	// VOLTAGE 
	#define V1 A1
	#define V2 A2
	#define V3 A3
	#define V4 A4

#else
	#error "No valid board version selected"
#endif

/*
 * INCLUDE CLASS
 */
#include "main.h"

#endif
