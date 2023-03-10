/*
 * LOCO-CAN module configuration file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */


#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#include "loco_config.h"
#include "can_protocol.h"
#include "can_com.h"


// =====================================
// The main module methods
// are declared in module.h
//
// The board definitions are
// defined in module_board.h
//
// Valid board versions:
// 
#define BOARD_V_1_0 1 // V1.0
#define BOARD_V_1_2 2 // V1.2
#define BOARD_V_2_0 3 // V2.0
#define BOARD_V_2_1 4 // V2.1
#define BOARD_V_2_1A 5 // V2.1a
#define BOARD_UNIV_V_2_X 6 // Universal V2.x
// =====================================


// =====================================
//
// select board version
//
// =====================================
#define BOARD_VERSION BOARD_UNIV_V_2_X
#define SOFTWARE_VERSION 0x0100

#include "module_board.h"


// =====================================
//
// local definitions
//
// =====================================

// All local definitions are placed here


// =====================================
//
// settings definitions
//
// =====================================

// max settings bytes
// #define MODULE_MAX_SETTINGS 1


#endif
