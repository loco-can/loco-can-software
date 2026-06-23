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
 * The module version is set in the config.h file
 */

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

#if MODULE_VERSION == V_1_0

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR
	#define ANALOGSWITCH_MAX_POS 1024

	#define CAN_RX 17
	#define CAN_TX 18
	#define CAN_STATUS_LED 46
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8

	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	// ======================================
	// CONTROL FUNCTION PARAMETERS
	// digital intput/output
	// not used
	#define SWITCH_PORT_COUNT 6

	#define LIGHT1 2
	#define LIGHT2 3
	#define LIGHT3 4
	#define LIGHT4 5
	#define LIGHT5 6
	#define LIGHT6 7


	// analog input
	// CURRENT
	#define C1 A0

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

#elif MODULE_VERSION == BOARD_V_1_2

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_AVR
	#define ANALOGSWITCH_MAX_POS 1024

	#define CAN_RX 17
	#define CAN_TX 18
	#define CAN_STATUS_LED 7
	#define CAN_MAX_FILTER 8
	#define CAN_BUFFER_SIZE 8

	#define UI_MAX_VOLT 4
	#define UI_MAX_CURR 1
	#define UI_MAX_DIGITAL 8
	#define UI_MAX_PWM 3

	// digital intput/output
	// not used
	#define DIGI1 D2
	#define DIGI2 D3
	#define DIGI3 D4
	#define DIGI4 D5
	#define DIGI5 D6
	#define DIGI6 D7
	#define DIGI7 D8
	#define DIGI8 D9

	#define PWM1 D3
	#define PWM2 D6
	#define PWM3 D9
	#define PWM4 -1
	#define PWM5 -1
	#define PWM6 -1

	// analog input
	// CURRENT
	#define C1 A0
	#define C2 -1
	#define C3 -1
	#define C4 -1
	#define C5 -1
	#define C6 -1

	// VOLTAGE 
	#define V1 -1
	#define V2 A1
	#define V3 A2
	#define V4 A3
	#define V5 -1
	#define V6 -1

#elif BOARD_VERSION == BOARD_V_2_0

	#define CAN_STATUS_LED 8
	#define CAN_STATUS_LED_WRITE 9

	#define ANALOGSWITCH_MAX_POS 1024


	// digital intput/output
	// not used
	#define SWITCH_PORT_COUNT 6

	#define LIGHT1 2
	#define LIGHT2 3
	#define LIGHT3 4
	#define LIGHT4 5
	#define LIGHT5 6
	#define LIGHT6 7

	// analog input
	// CURRENT
	#define C1 A0

#else
	#error "No valid board version selected"
#endif

/*
 * INCLUDE CLASS
 */
#include "main.h"

#endif
