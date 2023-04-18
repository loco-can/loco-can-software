/*
 * LOCO-CAN Voltage/Current Module board configuration
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */


#ifndef MODULE_BOARD_H
#define MODULE_BOARD_H

#include "config.h"

#ifndef BOARD_VERSION
	#error "no board defined"
#endif

/*
 * The bus connector offers seven IO-Lines depending on the used board
 */

// unsupported lines are set to -1

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

#if BOARD_VERSION == BOARD_DIGISPARK

	#define CAN_STATUS_LED 1

	#define ANALOG_RESOLUTION 10

	// digital intput/output
	#define IO_1 2
	#define IO_2 3



// ATtiny85 pinout						Loco-Micro
//	1	D5	ADC0	RESET			|	I/O-2
//	2	D3	ADC3	USB+			|	USB+	I/O-1
//	3	D4	ADC2	USB-	PWM4	|	USB-	CS
//	4 	GND
//	5	PB0	PWM0	MOSI	SDA		|	SI
//	6	D1	PWM1	MISO			|	SO
//	7	D2	ADC1	SCLK	SCL		|	SCK
//	8 	5V



#elif BOARD_VERSION == BOARD_MICRO_V_2_2

	#define CAN_STATUS_LED 1

	#define ANALOG_RESOLUTION 10

	// digital intput/output
	#define IO_1 2
	#define IO_2 3

#else
	#error "No valid board version selected"
#endif

#endif