/*
 * Loco-CAN module parameters
 *
 * @author: Thomas H Winkler
 * @copyright: 2026
 * @lizence: GG0
 *
 * The parameter structures for all modules are defined in this file.
 */
#pragma once

#ifnded PARAMETER_H
#define PARAMETER_H

#include <inttypes.h>

// =======================================
// common module data
struct MODULE_TYPE {
	uint8_t type; // module type id
	uint16_t software_version;
	uint16_t hardware_version;
};


// =======================================
// CONTROLLER module parameters definition
#ifdef CONTROLLER_MODULE_CLASS

struct PARAM_CONTROLLER {
	MODULE_TYPE type;
	uint16_t mains_points[ANALOGSWITCH_MAX_POS]; // settings for mains analogswitch
	uint16_t dir_points[3]; // settings of direction analogswitch
	uint8_t status_mode; // led, oled status type
	uint8_t drive_mode; // drive mode: single, drive/break, speed/power/break
};

#endif

// =======================================


// =======================================
// DRIVE parameters definition

#ifdef DRIVE_MODULE_CLASS

struct PARAM_DRIVE {
	MODULE_TYPE type;
	uint16_t mains_points[ANALOGSWITCH_MAX_POS]; // settings for mains analogswitch
	uint16_t dir_points[3]; // settings of direction analogswitch
	uint8_t status_mode; // led, oled status type
	uint8_t drive_mode; // drive mode: single, drive/break, speed/power/break
	
	uint16_t paired; // uuid of paired controller
	uint16_t multi_traction; // 0=single, 1=leading engine, 2=supporting engine
	uint16_t tacho_pulses; // tacho pulses per meter
	uint16_t battery_type; // battery type (1x12V/24V, 2x12V in serie) 
};

#endif

// =======================================


// =======================================
// electro locomotive parameters definition

#ifdef ELECTRIC_MODULE_CLASS

struct PARAM_ELECTRIC {
	MODULE_TYPE type;
	uint16_t paired; // uuid of paired controller
	uint16_t multi_traction; // 0=single, 1=leading engine, 2=supporting engine
	uint16_t tacho_pulses; // tacho pulses per meter
	uint16_t battery_type; // battery type (1x12V/24V, 2x12V in serie) 
};

#endif

// =======================================


// =======================================
// sensor parameters definition

#ifdef SENSOR_MODULE_CLASS

struct PARAM_SENSOR {
	MODULE_TYPE type;
};

#endif

// =======================================


// =======================================
// servo parameters definition

#ifdef SERVO_MODULE_CLASS

struct PARAM_SERVO {
	MODULE_TYPE type;
};

#endif

// =======================================


// =======================================
// switch parameters definition

#ifdef SWITCH_MODULE_CLASS

struct SWITCH_MAP {
	uint8_t port; // output port
	uint16_t can_id; // message id to use for switching
	uint8_t byte; // byte index of message
	uint8_t bit; // bit to be used to switch
	bool invert; // invert bit for switch state
};

struct PARAM_SWITCH {
	MODULE_TYPE type;
	SWITCH_MAP output[SWITCH_PORT_COUNT];
};

#endif

// =======================================

#endif
