/*
 * LOCO-CAN Servo Module board configuration
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */


#ifndef MODULE_BOARD_H
#define MODULE_BOARD_H

#include "config.h"

/*
 * The bus connector offers seven IO-Lines depending on the used board
 */

// unsupported lines are set to -1
#if BOARD_VERSION == BOARD_V_2_0

	#define CAN_STATUS_LED 7

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

#endif