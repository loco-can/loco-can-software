/*
 * Loco-CAN
 * 
 * @author: 
 * @copyright: 
 * @lizence: GG0
 *
 * Version 3.x
 */

 /* *************************************************
 * module definition and board settings
 */

#pragma once


/* ******************************************
 * The module version is set in the config.h file
 */

#if MODULE_VERSION == V_2_x

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_PLATFORM_ATMEGA
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




	#include "../function/controller/main.h"	// pot input



/*
 * ESP32S3 based version
 * not yet implemented, only for debugging use
 */
#elif MODULE_VERSION == V_3_x

	#define MODULE_PLATFORM_ESP32
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

	// FUNCTION PARAMETERS




	#include "../function/controller/main.h"	// pot input

#endif
