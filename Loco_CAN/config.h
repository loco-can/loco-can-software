/*
 * LOCO-DRIVE module configuration file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */


#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>


/* =====================================
 * The main module methods
 * are declared in module.h
 *
 * The board definitions are
 * defined in module_board.h
 *
 * Valid board versions:
 */

/* ================================
 * ATMEGA based boards V2.x
 * ESP32 based boards V3.x
 */


// universal module
#define UNIVERSAL_BOARD_V_2_0 0x20
#define UNIVERSAL_BOARD_V_2_1 0x21

// controller module
#define CONTROLLER_BOARD_V_2_0 0x20
#define CONTROLLER_BOARD_V_2_1 0x21
#define CONTROLLER_BOARD_V_3_0 0x30

// six MOSFET switched outputs (ex LIGHT BOARD)
#define SWITCH_BOARD_V_2_0 0x20
#define SWITCH_BOARD_V_2_1 0x21

// 50A current, 4-pin Voltage, pulse sensor
#define SENSOR_BOARD_V_2_0 0x20

// servo module for 4 model servos
#define SERVO_BOARD_V_2_0 0x20
#define SERVO_BOARD_V_2_1 0x21

// motor adapter to connect to power driver
#define MOTOR_BOARD_V_2_0 0x20
#define MOTOR_BOARD_V_2_1 0x21

// 8 LED drivers
#define LED_BOARD_V_3_0 0x30

// 4q motor driver
#define DRIVE_BOARD_V_3_0 0x30
#define DRIVE_BOARD_V_3_1 0x31


// =====================================
//
// select board version
//
// =====================================
// #define BOARD_VERSION DRIVE_BOARD_V_3_0
#define BOARD_VERSION CONTROLLER_BOARD_V_2_1

#include "boards.h"


// =====================================
//
// module settings definitions
//
// =====================================

// max settings bytes
// #define MODULE_MAX_SETTINGS 1


#endif
