/*
 * LOCO-CAN ESP32 WIFI Module board configuration
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */

#ifndef MODULE_UI_BOARD_H
#define MODULE_UI_BOARD_H

#include "config.h"


// BOARD VERSION 2.0
// 
// BUS 1				BUS 2
// 
// 9x1 pins             5+4x1 pins
//
//  1   +3,3V			1   VCC (12V)
//  2   IO27			2   IO4
//  3   IO26			3   IO0 (LOW: enter flash)
//  4   IO25			4   IO2
//  5   IO33			5   IO15
//  6   IO32
//  7   IO35 (input)	1   IO13
//  8   IO34 (input)	2   IO12
//  9   GND				3   IO14
// 						4   GND


// IO0: LOW to enter flashing mode
// input only: 34, 35, 36, 39

// IO 36:   CAN-RX  (WROOM pin 4)
// IO 9:   CAN-TX  (WROOM pin 18)


#if BOARD_VERSION == BOARD_CONTROL_V2_0

	#define CAN_STATUS_LED 16
    #define WIFI_STATUS_LED 16

    #define CAN_RX  21
    #define CAN_TX  22

#endif

#endif