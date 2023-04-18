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

#include "can_protocol.h"
#include "loco_config.h"
#include "can_com.h"

#include "settings.h"

#define DEBUG

// =====================================
// The main module methods
// are declared in module.h
//
// The board definitions are
// defined in module_board.h
//
// Valid board versions are defined in the LocoCAN.h library
// =====================================


// =====================================
//
// select board version
//
// =====================================

#define BOARD_VERSION BOARD_MICRO_V_2_2
#define SOFTWARE_VERSION 0x0100

#include "module_board.h"

// =====================================
//
// local definitions
//
// =====================================

// All local definitions are placed here
// value send interval ms
#define LIGHT_CURRENT_TIME 100


// =====================================
//
// settings definitions
//
// =====================================

#include "module_settings.h"

// max settings bytes
#define MODULE_MAX_SETTINGS 23

#endif
