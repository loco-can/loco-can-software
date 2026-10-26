/*
 * Motor-controller plugins for the electric module.
 *
 * The module speaks one CAN drive message. A plugin turns that message
 * into the signals of a particular controller:
 *
 *   direct  power stage with drive PWM, brake PWM, forward and reverse
 *           relays, and an optional relay that cuts the motor power line
 *   4qd     throttle PWM, enable, reverse switch, ignition
 *   curtis  throttle PWM, forward and reverse switches, keyswitch,
 *           high-pedal disable
 *
 * Direction relays change only while |motor voltage| is under the
 * configured minimum. Until then the direct plugin opens the power
 * relay and holds the previous direction.
 */
#pragma once

#ifndef ELECTRIC_PLUGIN_H
#define ELECTRIC_PLUGIN_H

#include <stdint.h>

#define ELECTRIC_PLUGIN_DIRECT 0
#define ELECTRIC_PLUGIN_4QD 1
#define ELECTRIC_PLUGIN_CURTIS 2
#define ELECTRIC_PLUGIN_COUNT 3

#define ELECTRIC_PWM_DRIVE 0
#define ELECTRIC_PWM_DRIVE_BRAKE 1

#define ELECTRIC_DIR_NONE 0
#define ELECTRIC_DIR_FORWARD 1
#define ELECTRIC_DIR_REVERSE 2

/* 10-bit throttle at or below this counts as zero (Curtis HPD). */
#define ELECTRIC_THROTTLE_ZERO 16

struct ELECTRIC_COMMAND {
	bool present;
	bool mains;
	bool motor;
	bool reverse;
	bool error;
	bool emergency;
	bool multi;
	uint16_t loco;
	uint16_t drive;
	uint16_t brake;
	uint16_t power;
};

struct ELECTRIC_PLUGIN_INPUT {
	uint8_t pwm_mode;
	bool power_relay_fitted;
	uint16_t voltage_min;
	int32_t motor_voltage;
	bool reversed;
	/*
	 * True after the relays have been open long enough to finish a
	 * direction change. The first engage, with both relays already
	 * open, does not wait.
	 */
	bool gap_elapsed;
	ELECTRIC_COMMAND command;
};

struct ELECTRIC_PLUGIN_STATE {
	uint8_t direction;
	uint8_t pending;
	bool hpd_locked;
	bool was_enabled;
};

struct ELECTRIC_OUTPUT {
	uint8_t drive_pwm;
	uint8_t brake_pwm;
	bool forward;
	bool reverse;
	bool power;
	bool direction_blocked;
	uint8_t direction;
};

void electric_plugin_reset(ELECTRIC_PLUGIN_STATE &state);

/* Unknown plugin ids produce a safe, fully off output. */
ELECTRIC_OUTPUT electric_plugin_apply(
	uint8_t plugin,
	const ELECTRIC_PLUGIN_INPUT &in,
	ELECTRIC_PLUGIN_STATE &state
);

/* Inverse of controller_pack_drive(). Returns false when size < 8. */
bool electric_unpack_drive(const uint8_t *data, uint8_t size, ELECTRIC_COMMAND &command);

struct ELECTRIC_STATUS_BITS {
	bool error;
	bool ready;
	bool moving;
	bool multi;
	bool reverse_logic;
	bool reverse_dir;
	bool drive;
	bool mains;
};

uint8_t electric_status_byte(const ELECTRIC_STATUS_BITS &bits);

ELECTRIC_STATUS_BITS electric_vehicle_status(
	const ELECTRIC_COMMAND &command,
	const ELECTRIC_OUTPUT &output,
	int32_t motor_voltage,
	uint16_t voltage_min,
	bool reverse_logic,
	bool link_fault,
	bool setup_idle
);

#endif
