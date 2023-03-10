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

#include "settings.h"


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
// =====================================


// =====================================
//
// select board version
//
// =====================================

#define BOARD_VERSION BOARD_V_2_0
#define SOFTWARE_VERSION 0x0010

#include "module_board.h"

// =====================================
//
// local definitions
//
// =====================================

// All local definitions are placed here
// value send interval ms



// =====================================
//
// settings definitions
//
// =====================================

// max settings bytes
#define MODULE_MAX_SETTINGS 1


#endif
