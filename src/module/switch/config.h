// TO BE MODIFIED


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
 * switch board settings
 */
#pragma once

#ifndef MODULE_SWITCH_CONFIG_H
#define MODULE_SWITCH_CONFIG_H


/* ******************************************
 * The module version is set in the config.h file.
 * Skip the pin map when another module is selected.
 */

#if defined(SWITCH_MODULE_VERSION) && SWITCH_MODULE_VERSION == V_1_0

	// BOARD VERSION 1.0
	//
	//	4x analog in
	//	4x digital IO
	//	2x PWM
	//
	// Print V1.0 extension bus
	//
	// 10x1 pins
	//
	//	1	VCC
	//	2	D9	(PWM)
	//	3	D8
	//	4	D7
	//	5	D6	(PWM)
	//	6	A0
	//	7	A1
	//	8	A2
	//	9	A3
	//	10	GND

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_SS 10
	// #define CAN_INT 18
	#define CAN_STATUS_LED 9
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8

	// ======================================
	// CONTROL FUNCTION PARAMETERS
	// digital outputs, mapped to a CAN bit (see function.h)
	// defaults: light_low_front, light_low_back,
	// light_back_front, light_back_back, horn_low, horn_high
	#define SWITCH_PORT_COUNT 6

	#define SWITCH1 2
	#define SWITCH2 3
	#define SWITCH3 4
	#define SWITCH4 5
	#define SWITCH5 6
	#define SWITCH6 7

	// analog input
	// module current, full-scale ADC = milliamps
	#define C1 A0
	#define SWITCH_CURRENT_PORT C1
	#define SWITCH_CURRENT_FULL_SCALE_MA 30000


#elif defined(SWITCH_MODULE_VERSION) && SWITCH_MODULE_VERSION == V_2_0

	// Print V1.2 extension bus
	// 8x2 pins
	//
	//	1	VCC			2	D2
	//	3	A5			4	D3 (PWM)
	//	5	A4			6	D4
	//	7	A3			8	D5 (PWM)
	//	8	A2			10	D6 (PWM)
	//	11	A1			12	D7
	//	13	A0			14	D8
	//	15	D9 (PWM)	16	GND

	// big extension bus version
	// 9x2 pins
	//  ...
	//	17	+12V		18	GND

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_SS 10
	// #define CAN_INT 18
	#define CAN_STATUS_LED 9
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8

	// ======================================
	// CONTROL FUNCTION PARAMETERS
	// digital outputs, mapped to a CAN bit (see function.h)
	// defaults: light_low_front, light_low_back,
	// light_back_front, light_back_back, horn_low, horn_high
	#define SWITCH_PORT_COUNT 6

	#define SWITCH1 2
	#define SWITCH2 3
	#define SWITCH3 4
	#define SWITCH4 5
	#define SWITCH5 6
	#define SWITCH6 7

	// analog input
	// module current, full-scale ADC = milliamps
	#define C1 A0
	#define SWITCH_CURRENT_PORT C1
	#define SWITCH_CURRENT_FULL_SCALE_MA 30000

#elif defined(SWITCH_MODULE_VERSION) && SWITCH_MODULE_VERSION == V_2_1

	// Print V1.2 extension bus
	// 8x2 pins
	//
	//	1	VCC			2	D2
	//	3	A5			4	D3 (PWM)
	//	5	A4			6	D4
	//	7	A3			8	D5 (PWM)
	//	8	A2			10	D6 (PWM)
	//	11	A1			12	D7
	//	13	A0			14	D8
	//	15	D9 (PWM)	16	GND

	// big extension bus version
	// 9x2 pins
	//  ...
	//	17	+12V		18	GND

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR

	#define PLATFORM_ANALOG_RESOLUTION 1024
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_SS 10
	// #define CAN_INT 18
	#define CAN_STATUS_LED 9
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8

	// ======================================
	// CONTROL FUNCTION PARAMETERS
	// digital outputs, mapped to a CAN bit (see function.h)
	// defaults: light_low_front, light_low_back,
	// light_back_front, light_back_back, horn_low, horn_high
	#define SWITCH_PORT_COUNT 6

	#define SWITCH1 2
	#define SWITCH2 3
	#define SWITCH3 4
	#define SWITCH4 5
	#define SWITCH5 6
	#define SWITCH6 7

	// analog input
	// module current, full-scale ADC = milliamps
	#define C1 A0
	#define SWITCH_CURRENT_PORT C1
	#define SWITCH_CURRENT_FULL_SCALE_MA 30000

#elif defined(SWITCH_MODULE_VERSION)
	#error "No valid board version selected"
#endif

#if defined(SWITCH_MODULE_VERSION)
/*
 * INCLUDE CLASS
 */
#include "main.h"
#endif

#endif
