/*
 * Plugins that turn a Loco-CAN drive command into controller signals.
 */

#include "plugin.h"

#include "../../can_protocol.h"


void electric_plugin_reset(ELECTRIC_PLUGIN_STATE &state) {

	state.direction = ELECTRIC_DIR_NONE;
	state.pending = ELECTRIC_DIR_NONE;
	state.hpd_locked = true;
	state.was_enabled = false;
}


static uint16_t electric_clamp10(uint16_t value) {

	if (value > 1023) {
		return 1023;
	}

	return value;
}


static uint8_t electric_to_pwm(uint16_t value) {

	value = electric_clamp10(value);
	return (uint8_t)(((uint32_t)value * 255UL) / 1023UL);
}


static uint16_t electric_limited_drive(const ELECTRIC_COMMAND &command) {

	uint16_t drive = electric_clamp10(command.drive);
	uint16_t power = electric_clamp10(command.power);

	return (uint16_t)(((uint32_t)drive * power) / 1023UL);
}


static void electric_pwm_pair(uint8_t mode, uint16_t drive, uint16_t brake, bool drop_throttle_on_brake, uint8_t &drive_pwm, uint8_t &brake_pwm) {

	drive = electric_clamp10(drive);
	brake = electric_clamp10(brake);

	if (mode == ELECTRIC_PWM_DRIVE_BRAKE) {
		drive_pwm = electric_to_pwm(drive);
		brake_pwm = electric_to_pwm(brake);
		return;
	}

	brake_pwm = 0;

	if (drop_throttle_on_brake && brake > ELECTRIC_THROTTLE_ZERO) {
		drive_pwm = 0;
		return;
	}

	if (brake >= drive) {
		drive_pwm = 0;
		return;
	}

	drive_pwm = electric_to_pwm((uint16_t)(drive - brake));
}


static bool electric_voltage_low(int32_t motor_voltage, uint16_t voltage_min) {

	int32_t absolute = motor_voltage < 0 ? -motor_voltage : motor_voltage;
	return absolute < (int32_t)voltage_min;
}


static uint8_t electric_want_direction(const ELECTRIC_COMMAND &command, bool reversed) {

	if (!command.present || !command.mains || !command.motor || command.error || command.emergency) {
		return ELECTRIC_DIR_NONE;
	}

	bool reverse = command.reverse ^ reversed;
	return reverse ? ELECTRIC_DIR_REVERSE : ELECTRIC_DIR_FORWARD;
}


/*
 * Dropping to none is always allowed. Engaging or swapping a direction
 * waits until the motor voltage is under the minimum.
 * Returns true while the requested direction is not yet applied.
 */
static bool electric_settle_direction(uint8_t want, bool voltage_low, bool gap_elapsed, ELECTRIC_PLUGIN_STATE &state) {

	if (want == ELECTRIC_DIR_NONE) {
		state.direction = ELECTRIC_DIR_NONE;
		state.pending = ELECTRIC_DIR_NONE;
		return false;
	}

	if (want == state.direction) {
		state.pending = ELECTRIC_DIR_NONE;
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
	if (state.direction != ELECTRIC_DIR_NONE) {
		state.direction = ELECTRIC_DIR_NONE;
		state.pending = want;
		return true;
	}

	if (state.pending != ELECTRIC_DIR_NONE && !gap_elapsed) {
		state.pending = want;
		return true;
	}

	state.direction = want;
	state.pending = ELECTRIC_DIR_NONE;
	return false;
}


static void electric_clear_output(ELECTRIC_OUTPUT &out) {

	out.drive_pwm = 0;
	out.brake_pwm = 0;
	out.forward = false;
	out.reverse = false;
	out.power = false;
	out.direction_blocked = false;
	out.direction = ELECTRIC_DIR_NONE;
}


static void electric_update_hpd(bool enable, bool blocked, bool direction_changed, uint16_t drive, ELECTRIC_PLUGIN_STATE &state) {

	if (!enable) {
		state.hpd_locked = true;
		state.was_enabled = false;
		return;
	}

	if ((!state.was_enabled || direction_changed || blocked) && drive > ELECTRIC_THROTTLE_ZERO) {
		state.hpd_locked = true;
	}
	else if (state.hpd_locked && drive <= ELECTRIC_THROTTLE_ZERO) {
		state.hpd_locked = false;
	}

	state.was_enabled = true;
}


static ELECTRIC_OUTPUT electric_direct_apply(const ELECTRIC_PLUGIN_INPUT &in, ELECTRIC_PLUGIN_STATE &state) {

	ELECTRIC_OUTPUT out;
	electric_clear_output(out);

	uint8_t want = electric_want_direction(in.command, in.reversed);
	bool low = electric_voltage_low(in.motor_voltage, in.voltage_min);
	bool blocked = electric_settle_direction(want, low, in.gap_elapsed, state);
	bool enable = want != ELECTRIC_DIR_NONE;

	uint16_t drive = electric_limited_drive(in.command);
	uint16_t brake = electric_clamp10(in.command.brake);
	uint8_t drive_pwm = 0;
	uint8_t brake_pwm = 0;
	electric_pwm_pair(in.pwm_mode, drive, brake, false, drive_pwm, brake_pwm);

	out.direction = state.direction;
	out.direction_blocked = blocked;
	out.forward = state.direction == ELECTRIC_DIR_FORWARD;
	out.reverse = state.direction == ELECTRIC_DIR_REVERSE;

	if (!enable || blocked) {
		out.drive_pwm = 0;
		out.brake_pwm = (blocked && in.pwm_mode == ELECTRIC_PWM_DRIVE_BRAKE) ? brake_pwm : 0;
		out.power = false;
		return out;
	}

	out.drive_pwm = drive_pwm;
	out.brake_pwm = brake_pwm;
	out.power = in.power_relay_fitted;
	return out;
}


static ELECTRIC_OUTPUT electric_4qd_apply(const ELECTRIC_PLUGIN_INPUT &in, ELECTRIC_PLUGIN_STATE &state) {

	ELECTRIC_OUTPUT out;
	electric_clear_output(out);

	uint8_t want = electric_want_direction(in.command, in.reversed);
	bool low = electric_voltage_low(in.motor_voltage, in.voltage_min);
	bool blocked = electric_settle_direction(want, low, in.gap_elapsed, state);
	bool enable = want != ELECTRIC_DIR_NONE;

	uint16_t drive = electric_limited_drive(in.command);
	uint16_t brake = electric_clamp10(in.command.brake);
	uint8_t drive_pwm = 0;
	uint8_t brake_pwm = 0;
	electric_pwm_pair(in.pwm_mode, drive, brake, true, drive_pwm, brake_pwm);

	out.direction = state.direction;
	out.direction_blocked = blocked;
	out.reverse = state.direction == ELECTRIC_DIR_REVERSE;

	/*
	 * 4QD uses one reverse switch and a separate enable. FORW is that
	 * enable. Ignition (the optional relay) drops while a direction
	 * change is waiting for the motor voltage to collapse.
	 */
	if (!enable || blocked) {
		out.forward = false;
		out.drive_pwm = 0;
		out.brake_pwm = (blocked && in.pwm_mode == ELECTRIC_PWM_DRIVE_BRAKE) ? brake_pwm : 0;
		out.power = false;
		if (!enable) {
			out.reverse = false;
			out.direction = ELECTRIC_DIR_NONE;
		}
		return out;
	}

	out.forward = true;
	out.drive_pwm = drive_pwm;
	out.brake_pwm = brake_pwm;
	out.power = in.power_relay_fitted;
	return out;
}


static ELECTRIC_OUTPUT electric_curtis_apply(const ELECTRIC_PLUGIN_INPUT &in, ELECTRIC_PLUGIN_STATE &state) {

	ELECTRIC_OUTPUT out;
	electric_clear_output(out);

	uint8_t before = state.direction;
	uint8_t want = electric_want_direction(in.command, in.reversed);
	bool low = electric_voltage_low(in.motor_voltage, in.voltage_min);
	bool blocked = electric_settle_direction(want, low, in.gap_elapsed, state);
	bool enable = want != ELECTRIC_DIR_NONE;
	bool changed = state.direction != before;

	uint16_t drive = electric_limited_drive(in.command);
	uint16_t brake = electric_clamp10(in.command.brake);
	electric_update_hpd(enable, blocked, changed, drive, state);

	uint8_t drive_pwm = 0;
	uint8_t brake_pwm = 0;
	electric_pwm_pair(in.pwm_mode, drive, brake, false, drive_pwm, brake_pwm);

	if (state.hpd_locked) {
		drive_pwm = 0;
	}

	out.direction = state.direction;
	out.direction_blocked = blocked;
	out.forward = state.direction == ELECTRIC_DIR_FORWARD;
	out.reverse = state.direction == ELECTRIC_DIR_REVERSE;

	if (!enable) {
		out.drive_pwm = 0;
		out.brake_pwm = 0;
		out.forward = false;
		out.reverse = false;
		out.power = false;
		out.direction = ELECTRIC_DIR_NONE;
		return out;
	}

	if (blocked) {
		out.drive_pwm = 0;
		out.brake_pwm = (in.pwm_mode == ELECTRIC_PWM_DRIVE_BRAKE) ? brake_pwm : 0;
	}
	else {
		out.drive_pwm = drive_pwm;
		out.brake_pwm = brake_pwm;
	}

	/* Keyswitch stays on while the controller command is enabled. */
	out.power = in.power_relay_fitted;
	return out;
}


ELECTRIC_OUTPUT electric_plugin_apply(uint8_t plugin, const ELECTRIC_PLUGIN_INPUT &in, ELECTRIC_PLUGIN_STATE &state) {

	switch (plugin) {
		case ELECTRIC_PLUGIN_DIRECT:
			return electric_direct_apply(in, state);
		case ELECTRIC_PLUGIN_4QD:
			return electric_4qd_apply(in, state);
		case ELECTRIC_PLUGIN_CURTIS:
			return electric_curtis_apply(in, state);
		default:
			break;
	}

	electric_plugin_reset(state);
	ELECTRIC_OUTPUT out;
	electric_clear_output(out);
	return out;
}


bool electric_unpack_drive(const uint8_t *data, uint8_t size, ELECTRIC_COMMAND &command) {

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


uint8_t electric_status_byte(const ELECTRIC_STATUS_BITS &bits) {

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


static bool electric_is_moving(const ELECTRIC_OUTPUT &output, int32_t motor_voltage, uint16_t voltage_min) {

	if (output.drive_pwm > 0 || output.brake_pwm > 0) {
		return true;
	}

	return !electric_voltage_low(motor_voltage, voltage_min);
}


ELECTRIC_STATUS_BITS electric_vehicle_status(
	const ELECTRIC_COMMAND &command,
	const ELECTRIC_OUTPUT &output,
	int32_t motor_voltage,
	uint16_t voltage_min,
	bool reverse_logic,
	bool link_fault,
	bool setup_idle
) {

	ELECTRIC_STATUS_BITS bits;
	bits.error = false;
	bits.ready = false;
	bits.moving = false;
	bits.multi = false;
	bits.reverse_logic = reverse_logic;
	bits.reverse_dir = output.direction == ELECTRIC_DIR_REVERSE;
	bits.drive = false;
	bits.mains = false;

	if (setup_idle && !link_fault && !command.emergency && !command.error) {
		bits.mains = true;
		bits.drive = true;
		bits.ready = true;
		return bits;
	}

	bits.error = link_fault || command.error || command.emergency;
	bits.mains = command.present && command.mains && !bits.error;
	bits.drive = bits.mains && command.motor;
	bits.ready = bits.drive && !output.direction_blocked;
	bits.moving = electric_is_moving(output, motor_voltage, voltage_min);
	bits.multi = command.multi;

	if (bits.error) {
		bits.ready = false;
		bits.drive = false;
		bits.mains = false;
	}

	return bits;
}
