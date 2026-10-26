/*
 * Motor-controller plugins for the vehicle module.
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

#ifndef VEHICLE_PLUGIN_H
#define VEHICLE_PLUGIN_H

#include <stdint.h>

#define VEHICLE_PLUGIN_DIRECT 0
#define VEHICLE_PLUGIN_4QD 1
#define VEHICLE_PLUGIN_CURTIS 2
#define VEHICLE_PLUGIN_COUNT 3

#define VEHICLE_PWM_DRIVE 0
#define VEHICLE_PWM_DRIVE_BRAKE 1

#define VEHICLE_DIR_NONE 0
#define VEHICLE_DIR_FORWARD 1
#define VEHICLE_DIR_REVERSE 2

/* 10-bit throttle at or below this counts as zero (Curtis HPD). */
#define VEHICLE_THROTTLE_ZERO 16

struct VEHICLE_COMMAND {
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

struct VEHICLE_PLUGIN_INPUT {
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
	VEHICLE_COMMAND command;
};

struct VEHICLE_PLUGIN_STATE {
	uint8_t direction;
	uint8_t pending;
	bool hpd_locked;
	bool was_enabled;
};

struct VEHICLE_OUTPUT {
	uint8_t drive_pwm;
	uint8_t brake_pwm;
	bool forward;
	bool reverse;
	bool power;
	bool enable;
	bool direction_blocked;
	uint8_t direction;
};

void vehicle_plugin_reset(VEHICLE_PLUGIN_STATE &state);

/* Unknown plugin ids produce a safe, fully off output. */
VEHICLE_OUTPUT vehicle_plugin_apply(
	uint8_t plugin,
	const VEHICLE_PLUGIN_INPUT &in,
	VEHICLE_PLUGIN_STATE &state
);

/* Inverse of controller_pack_drive(). Returns false when size < 8. */
bool vehicle_unpack_drive(const uint8_t *data, uint8_t size, VEHICLE_COMMAND &command);

/* True when 0x220 has SIGNAL_LOW or SIGNAL_HIGH set. */
bool vehicle_horn_from_signal(const uint8_t *data, uint8_t size);

/* True when 0x410 has LIGHT_LOW or LIGHT_HIGH set. */
bool vehicle_light_from_frame(const uint8_t *data, uint8_t size);

/*
 * DRIVE_ENABLE latch: set when the cab motor bit is live, cleared only
 * after mains is off and |motor voltage| is under voltage_min.
 */
bool vehicle_drive_enable(
	const VEHICLE_COMMAND &command,
	int32_t motor_voltage,
	uint16_t voltage_min,
	bool previous
);

struct VEHICLE_STATUS_BITS {
	bool error;
	bool ready;
	bool moving;
	bool multi;
	bool reverse_logic;
	bool reverse_dir;
	bool drive;
	bool mains;
};

uint8_t vehicle_status_byte(const VEHICLE_STATUS_BITS &bits);

VEHICLE_STATUS_BITS vehicle_status(
	const VEHICLE_COMMAND &command,
	const VEHICLE_OUTPUT &output,
	int32_t motor_voltage,
	uint16_t voltage_min,
	bool reverse_logic,
	bool link_fault,
	bool setup_idle
);

#endif
