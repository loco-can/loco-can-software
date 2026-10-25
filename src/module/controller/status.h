/*
 * Controller status values and the transitions between them.
 *
 * The inputs are the local switches plus what the CAN bus reports
 * (another controller's heartbeat, vehicle moving / error).
 */
#pragma once

#ifndef CONTROLLER_STATUS_H
#define CONTROLLER_STATUS_H

#include <stdint.h>

#define CONTROLLER_STATUS_OFF 0
#define CONTROLLER_STATUS_LOCKED 1
#define CONTROLLER_STATUS_STANDBY 2
#define CONTROLLER_STATUS_NULLED 3
#define CONTROLLER_STATUS_READY 4
#define CONTROLLER_STATUS_MOVING 5
#define CONTROLLER_STATUS_SETUP 6
#define CONTROLLER_STATUS_ON 7

/* Analog-switch positions, index 0 = first programmed point. */
#define CONTROLLER_MAINS_OFF 0
#define CONTROLLER_MAINS_ON 1
#define CONTROLLER_MAINS_MOTOR 2

#define CONTROLLER_DIR_REVERSE 0
#define CONTROLLER_DIR_NEUTRAL 1
#define CONTROLLER_DIR_FORWARD 2

#define CONTROLLER_MAX_VEHICLES 6

/* Drive lever at or below this 10-bit value counts as zero. */
#define CONTROLLER_ZERO_THRESHOLD 16

struct CONTROLLER_INPUTS {
	uint8_t mains;
	uint8_t dir;
	bool horn;
	bool horn2;
	bool drive_zero;
	bool bus_free;
	bool foreign;
	bool bus_moving;
	bool emergency;
	bool vehicle_error;
	bool fault_latched;
};

/* Two programmed points are reverse / forward. Three are reverse / neutral / forward. */
inline uint8_t controller_dir_from_switch(uint8_t index, uint8_t count) {

	if (count < 3 && index == 1) {
		return CONTROLLER_DIR_FORWARD;
	}

	return index;
}

inline bool controller_is_commanding(uint8_t status) {
	return status != CONTROLLER_STATUS_OFF && status != CONTROLLER_STATUS_LOCKED;
}

inline bool controller_direction_selected(uint8_t dir) {
	return dir != CONTROLLER_DIR_NEUTRAL;
}

inline bool controller_motor_on(uint8_t status) {
	return status == CONTROLLER_STATUS_READY
		|| status == CONTROLLER_STATUS_MOVING;
}

inline bool controller_transmits_speed(uint8_t status) {
	return status == CONTROLLER_STATUS_READY || status == CONTROLLER_STATUS_MOVING;
}

uint8_t controller_next_status(uint8_t status, const CONTROLLER_INPUTS &in, bool &nulled);

#endif
