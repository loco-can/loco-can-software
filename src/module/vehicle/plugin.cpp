/*
 * Plugins that turn a Loco-CAN drive command into controller signals.
 */

#include "plugin.h"

#include "../../can_protocol.h"


void vehicle_plugin_reset(VEHICLE_PLUGIN_STATE &state) {

	state.direction = VEHICLE_DIR_NONE;
	state.pending = VEHICLE_DIR_NONE;
	state.hpd_locked = true;
	state.was_enabled = false;
}


static uint16_t vehicle_clamp10(uint16_t value) {

	if (value > 1023) {
		return 1023;
	}

	return value;
}


static uint8_t vehicle_to_pwm(uint16_t value) {

	value = vehicle_clamp10(value);
	return (uint8_t)(((uint32_t)value * 255UL) / 1023UL);
}


static uint16_t vehicle_limited_drive(const VEHICLE_COMMAND &command) {

	uint16_t drive = vehicle_clamp10(command.drive);
	uint16_t power = vehicle_clamp10(command.power);

	return (uint16_t)(((uint32_t)drive * power) / 1023UL);
}


static void vehicle_pwm_pair(uint8_t mode, uint16_t drive, uint16_t brake, bool drop_throttle_on_brake, uint8_t &drive_pwm, uint8_t &brake_pwm) {

	drive = vehicle_clamp10(drive);
	brake = vehicle_clamp10(brake);

	if (mode == VEHICLE_PWM_DRIVE_BRAKE) {
		drive_pwm = vehicle_to_pwm(drive);
		brake_pwm = vehicle_to_pwm(brake);
		return;
	}

	brake_pwm = 0;

	if (drop_throttle_on_brake && brake > VEHICLE_THROTTLE_ZERO) {
		drive_pwm = 0;
		return;
	}

	if (brake >= drive) {
		drive_pwm = 0;
		return;
	}

	drive_pwm = vehicle_to_pwm((uint16_t)(drive - brake));
}


static bool vehicle_voltage_low(int32_t motor_voltage, uint16_t voltage_min) {

	int32_t absolute = motor_voltage < 0 ? -motor_voltage : motor_voltage;
	return absolute < (int32_t)voltage_min;
}


static uint8_t vehicle_want_direction(const VEHICLE_COMMAND &command, bool reversed) {

	if (!command.present || !command.mains || !command.motor || command.error || command.emergency) {
		return VEHICLE_DIR_NONE;
	}

	bool reverse = command.reverse ^ reversed;
	return reverse ? VEHICLE_DIR_REVERSE : VEHICLE_DIR_FORWARD;
}


/*
 * Dropping to none is always allowed. Engaging or swapping a direction
 * waits until the motor voltage is under the minimum.
 * Returns true while the requested direction is not yet applied.
 */
static bool vehicle_settle_direction(uint8_t want, bool voltage_low, bool gap_elapsed, VEHICLE_PLUGIN_STATE &state) {

	if (want == VEHICLE_DIR_NONE) {
		state.direction = VEHICLE_DIR_NONE;
		state.pending = VEHICLE_DIR_NONE;
		return false;
	}

	if (want == state.direction) {
		state.pending = VEHICLE_DIR_NONE;
		return false;
	}

	if (!voltage_low) {
		state.pending = want;
		return true;
	}

	/*
	 * Leave the latched direction only while the motor voltage is low.
	 * Relays that were on stay open until gap_elapsed, so forward and
	 * reverse cannot overlap. The first engage does not wait.
	 */
	if (state.direction != VEHICLE_DIR_NONE) {
		state.direction = VEHICLE_DIR_NONE;
		state.pending = want;
		return true;
	}

	if (state.pending != VEHICLE_DIR_NONE && !gap_elapsed) {
		state.pending = want;
		return true;
	}

	state.direction = want;
	state.pending = VEHICLE_DIR_NONE;
	return false;
}


static void vehicle_clear_output(VEHICLE_OUTPUT &out) {

	out.drive_pwm = 0;
	out.brake_pwm = 0;
	out.forward = false;
	out.reverse = false;
	out.power = false;
	out.enable = false;
	out.direction_blocked = false;
	out.direction = VEHICLE_DIR_NONE;
}


static void vehicle_update_hpd(bool enable, bool blocked, bool direction_changed, uint16_t drive, VEHICLE_PLUGIN_STATE &state) {

	if (!enable) {
		state.hpd_locked = true;
		state.was_enabled = false;
		return;
	}

	if ((!state.was_enabled || direction_changed || blocked) && drive > VEHICLE_THROTTLE_ZERO) {
		state.hpd_locked = true;
	}
	else if (state.hpd_locked && drive <= VEHICLE_THROTTLE_ZERO) {
		state.hpd_locked = false;
	}

	state.was_enabled = true;
}


static VEHICLE_OUTPUT vehicle_direct_apply(const VEHICLE_PLUGIN_INPUT &in, VEHICLE_PLUGIN_STATE &state) {

	VEHICLE_OUTPUT out;
	vehicle_clear_output(out);

	uint8_t want = vehicle_want_direction(in.command, in.reversed);
	bool low = vehicle_voltage_low(in.motor_voltage, in.voltage_min);
	bool blocked = vehicle_settle_direction(want, low, in.gap_elapsed, state);
	bool enable = want != VEHICLE_DIR_NONE;

	uint16_t drive = vehicle_limited_drive(in.command);
	uint16_t brake = vehicle_clamp10(in.command.brake);
	uint8_t drive_pwm = 0;
	uint8_t brake_pwm = 0;
	vehicle_pwm_pair(in.pwm_mode, drive, brake, false, drive_pwm, brake_pwm);

	out.direction = state.direction;
	out.direction_blocked = blocked;
	out.forward = state.direction == VEHICLE_DIR_FORWARD;
	out.reverse = state.direction == VEHICLE_DIR_REVERSE;

	if (!enable || blocked) {
		out.drive_pwm = 0;
		out.brake_pwm = (blocked && in.pwm_mode == VEHICLE_PWM_DRIVE_BRAKE) ? brake_pwm : 0;
		out.power = false;
		return out;
	}

	out.drive_pwm = drive_pwm;
	out.brake_pwm = brake_pwm;
	out.power = in.power_relay_fitted;
	return out;
}


static VEHICLE_OUTPUT vehicle_4qd_apply(const VEHICLE_PLUGIN_INPUT &in, VEHICLE_PLUGIN_STATE &state) {

	VEHICLE_OUTPUT out;
	vehicle_clear_output(out);

	uint8_t want = vehicle_want_direction(in.command, in.reversed);
	bool low = vehicle_voltage_low(in.motor_voltage, in.voltage_min);
	bool blocked = vehicle_settle_direction(want, low, in.gap_elapsed, state);
	bool enable = want != VEHICLE_DIR_NONE;

	uint16_t drive = vehicle_limited_drive(in.command);
	uint16_t brake = vehicle_clamp10(in.command.brake);
	uint8_t drive_pwm = 0;
	uint8_t brake_pwm = 0;
	vehicle_pwm_pair(in.pwm_mode, drive, brake, true, drive_pwm, brake_pwm);

	out.direction = state.direction;
	out.direction_blocked = blocked;
	out.reverse = state.direction == VEHICLE_DIR_REVERSE;

	/*
	 * 4QD uses one reverse switch and a separate enable. FORW is that
	 * enable. Ignition (the optional relay) drops while a direction
	 * change is waiting for the motor voltage to collapse.
	 */
	if (!enable || blocked) {
		out.forward = false;
		out.drive_pwm = 0;
		out.brake_pwm = (blocked && in.pwm_mode == VEHICLE_PWM_DRIVE_BRAKE) ? brake_pwm : 0;
		out.power = false;
		if (!enable) {
			out.reverse = false;
			out.direction = VEHICLE_DIR_NONE;
		}
		return out;
	}

	out.forward = true;
	out.drive_pwm = drive_pwm;
	out.brake_pwm = brake_pwm;
	out.power = in.power_relay_fitted;
	return out;
}


static VEHICLE_OUTPUT vehicle_curtis_apply(const VEHICLE_PLUGIN_INPUT &in, VEHICLE_PLUGIN_STATE &state) {

	VEHICLE_OUTPUT out;
	vehicle_clear_output(out);

	uint8_t before = state.direction;
	uint8_t want = vehicle_want_direction(in.command, in.reversed);
	bool low = vehicle_voltage_low(in.motor_voltage, in.voltage_min);
	bool blocked = vehicle_settle_direction(want, low, in.gap_elapsed, state);
	bool enable = want != VEHICLE_DIR_NONE;
	bool changed = state.direction != before;

	uint16_t drive = vehicle_limited_drive(in.command);
	uint16_t brake = vehicle_clamp10(in.command.brake);
	vehicle_update_hpd(enable, blocked, changed, drive, state);

	uint8_t drive_pwm = 0;
	uint8_t brake_pwm = 0;
	vehicle_pwm_pair(in.pwm_mode, drive, brake, false, drive_pwm, brake_pwm);

	if (state.hpd_locked) {
		drive_pwm = 0;
	}

	out.direction = state.direction;
	out.direction_blocked = blocked;
	out.forward = state.direction == VEHICLE_DIR_FORWARD;
	out.reverse = state.direction == VEHICLE_DIR_REVERSE;

	if (!enable) {
		out.drive_pwm = 0;
		out.brake_pwm = 0;
		out.forward = false;
		out.reverse = false;
		out.power = false;
		out.direction = VEHICLE_DIR_NONE;
		return out;
	}

	if (blocked) {
		out.drive_pwm = 0;
		out.brake_pwm = (in.pwm_mode == VEHICLE_PWM_DRIVE_BRAKE) ? brake_pwm : 0;
	}
	else {
		out.drive_pwm = drive_pwm;
		out.brake_pwm = brake_pwm;
	}

	/* Keyswitch stays on while the controller command is enabled. */
	out.power = in.power_relay_fitted;
	return out;
}


VEHICLE_OUTPUT vehicle_plugin_apply(uint8_t plugin, const VEHICLE_PLUGIN_INPUT &in, VEHICLE_PLUGIN_STATE &state) {

	switch (plugin) {
		case VEHICLE_PLUGIN_DIRECT:
			return vehicle_direct_apply(in, state);
		case VEHICLE_PLUGIN_4QD:
			return vehicle_4qd_apply(in, state);
		case VEHICLE_PLUGIN_CURTIS:
			return vehicle_curtis_apply(in, state);
		default:
			break;
	}

	vehicle_plugin_reset(state);
	VEHICLE_OUTPUT out;
	vehicle_clear_output(out);
	return out;
}


bool vehicle_unpack_drive(const uint8_t *data, uint8_t size, VEHICLE_COMMAND &command) {

	if (data == 0 || size < 8) {
		return false;
	}

	command.mains = (data[0] & (uint8_t)(1 << CONTROL_MAINS_FLAG)) != 0;
	command.motor = (data[0] & (uint8_t)(1 << CONTROL_DRIVE_FLAG)) != 0;
	command.reverse = (data[0] & (uint8_t)(1 << CONTROL_DIR_FLAG)) != 0;
	command.error = (data[0] & (uint8_t)(1 << CONTROL_ERROR_FLAG)) != 0;

	command.drive = (uint16_t)(((data[2] & 0x03) << 8) | data[3]);
	command.power = (uint16_t)(((data[4] & 0x03) << 8) | data[5]);
	command.brake = (uint16_t)(((data[6] & 0x03) << 8) | data[7]);
	command.multi = (data[2] & 0x80) != 0;
	command.loco = (uint16_t)(
		((uint16_t)((data[2] >> 2) & 0x1F) << 11)
		| ((uint16_t)((data[4] >> 2) & 0x1F) << 6)
		| (uint16_t)((data[6] >> 2) & 0x3F)
	);

	return true;
}


bool vehicle_horn_from_signal(const uint8_t *data, uint8_t size) {

	if (data == 0 || size < 1) {
		return false;
	}

	uint8_t mask = (uint8_t)((1 << SIGNAL_LOW) | (1 << SIGNAL_HIGH));
	return (data[0] & mask) != 0;
}


bool vehicle_light_from_frame(const uint8_t *data, uint8_t size) {

	if (data == 0 || size < 1) {
		return false;
	}

	uint8_t mask = (uint8_t)((1 << LIGHT_LOW) | (1 << LIGHT_HIGH));
	return (data[0] & mask) != 0;
}


uint8_t vehicle_status_byte(const VEHICLE_STATUS_BITS &bits) {

	uint8_t value = 0;

	if (bits.error) value |= (uint8_t)(1 << ERROR_FLAG);
	if (bits.ready) value |= (uint8_t)(1 << READY_FLAG);
	if (bits.moving) value |= (uint8_t)(1 << MOVING_FLAG);
	if (bits.multi) value |= (uint8_t)(1 << MULTI_FLAG);
	if (bits.reverse_logic) value |= (uint8_t)(1 << REVERSE);
	if (bits.reverse_dir) value |= (uint8_t)(1 << DIR_FLAG);
	if (bits.drive) value |= (uint8_t)(1 << DRIVE_FLAG);
	if (bits.mains) value |= (uint8_t)(1 << MAINS_FLAG);

	return value;
}


bool vehicle_drive_enable(
	const VEHICLE_COMMAND &command,
	int32_t motor_voltage,
	uint16_t voltage_min,
	bool previous
) {

	if (command.present && command.motor && !command.emergency) {
		return true;
	}

	if (!command.mains && vehicle_voltage_low(motor_voltage, voltage_min)) {
		return false;
	}

	return previous;
}


static bool vehicle_is_moving(int32_t motor_voltage, uint16_t voltage_min) {

	return !vehicle_voltage_low(motor_voltage, voltage_min);
}


VEHICLE_STATUS_BITS vehicle_status(
	const VEHICLE_COMMAND &command,
	const VEHICLE_OUTPUT &output,
	int32_t motor_voltage,
	uint16_t voltage_min,
	bool reverse_logic,
	bool link_fault,
	bool setup_idle
) {

	VEHICLE_STATUS_BITS bits;
	bits.error = false;
	bits.ready = false;
	bits.moving = false;
	bits.multi = false;
	bits.reverse_logic = reverse_logic;
	bits.reverse_dir = output.direction == VEHICLE_DIR_REVERSE;
	bits.drive = false;
	bits.mains = false;

	if (setup_idle && !link_fault && !command.emergency) {
		bits.mains = true;
		bits.drive = true;
		bits.ready = true;
		return bits;
	}

	/*
	 * command.error is the cab telling us to cut. Echoing it on 0x500
	 * makes the cab see a vehicle fault, drop to OFF, send another
	 * error drive, and never recover. Cut happens in the plugin.
	 */
	bits.error = link_fault || command.emergency;
	bits.mains = command.present && command.mains && !bits.error;
	bits.drive = bits.mains && command.motor;
	bits.ready = bits.drive && !output.direction_blocked;
	bits.moving = vehicle_is_moving(motor_voltage, voltage_min);
	bits.multi = command.multi;

	if (bits.error) {
		bits.ready = false;
		bits.drive = false;
		bits.mains = false;
	}

	return bits;
}
