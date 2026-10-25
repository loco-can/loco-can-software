/*
 * Loco-CAN controller status transitions
 */

#include "status.h"


uint8_t controller_next_status(uint8_t status, const CONTROLLER_INPUTS &in, bool &nulled) {

	/*
	 * Horn / 0x000 cut every commanding state, including standby and
	 * loco-setup. A 0x500 error only cuts motor-on states. Standby must
	 * keep publishing mains=1 so the locomotive can clear a latched
	 * status error.
	 */
	if (in.emergency && controller_is_commanding(status)) {
		nulled = false;
		return CONTROLLER_STATUS_OFF;
	}
	if (in.vehicle_error && controller_motor_on(status)) {
		nulled = false;
		return CONTROLLER_STATUS_OFF;
	}

	switch (status) {

		case CONTROLLER_STATUS_OFF:
			if (in.mains == CONTROLLER_MAINS_OFF) {
				nulled = false;
				return CONTROLLER_STATUS_OFF;
			}
			/*
			 * Stay off after a fault until the mains switch is cycled.
			 * Vehicle error alone must not block standby after that
			 * cycle — otherwise the cab never sends a clean 0x400 and
			 * the locomotive cannot leave error.
			 */
			if (in.fault_latched) {
				return CONTROLLER_STATUS_OFF;
			}
			if (!in.bus_free) {
				if (in.foreign) {
					return CONTROLLER_STATUS_LOCKED;
				}
				return CONTROLLER_STATUS_OFF;
			}
			if ((in.horn || in.horn2) && !in.bus_moving) {
				nulled = false;
				return CONTROLLER_STATUS_SETUP;
			}
			nulled = in.drive_zero;
			return CONTROLLER_STATUS_STANDBY;

		case CONTROLLER_STATUS_LOCKED:
			if (in.mains == CONTROLLER_MAINS_OFF) {
				nulled = false;
				return CONTROLLER_STATUS_OFF;
			}
			if (in.bus_free) {
				nulled = in.drive_zero;
				return CONTROLLER_STATUS_STANDBY;
			}
			return CONTROLLER_STATUS_LOCKED;

		case CONTROLLER_STATUS_STANDBY:
			if (in.mains == CONTROLLER_MAINS_OFF) {
				nulled = false;
				return CONTROLLER_STATUS_OFF;
			}
			if (in.drive_zero) {
				nulled = true;
			}
			if (in.mains == CONTROLLER_MAINS_MOTOR) {
				if (!nulled) {
					return CONTROLLER_STATUS_NULLED;
				}
				return CONTROLLER_STATUS_ON;
			}
			return CONTROLLER_STATUS_STANDBY;

		case CONTROLLER_STATUS_NULLED:
			if (in.mains == CONTROLLER_MAINS_OFF) {
				nulled = false;
				return CONTROLLER_STATUS_OFF;
			}
			if (in.drive_zero) {
				nulled = true;
			}
			if (!nulled) {
				return CONTROLLER_STATUS_NULLED;
			}
			if (in.mains != CONTROLLER_MAINS_MOTOR) {
				return CONTROLLER_STATUS_STANDBY;
			}
			if (controller_direction_selected(in.dir)) {
				return CONTROLLER_STATUS_READY;
			}
			return CONTROLLER_STATUS_ON;

		case CONTROLLER_STATUS_ON:
			if (in.mains == CONTROLLER_MAINS_OFF) {
				nulled = false;
				return CONTROLLER_STATUS_OFF;
			}
			if (in.mains != CONTROLLER_MAINS_MOTOR) {
				return CONTROLLER_STATUS_STANDBY;
			}
			if (controller_direction_selected(in.dir)) {
				if (!nulled && !in.drive_zero) {
					nulled = false;
					return CONTROLLER_STATUS_NULLED;
				}
				nulled = true;
				return CONTROLLER_STATUS_READY;
			}
			return CONTROLLER_STATUS_ON;

		case CONTROLLER_STATUS_READY:
			if (in.mains == CONTROLLER_MAINS_OFF) {
				nulled = false;
				return CONTROLLER_STATUS_OFF;
			}
			if (in.bus_moving) {
				return CONTROLLER_STATUS_MOVING;
			}
			if (!controller_direction_selected(in.dir)) {
				return CONTROLLER_STATUS_ON;
			}
			if (in.mains != CONTROLLER_MAINS_MOTOR) {
				return CONTROLLER_STATUS_STANDBY;
			}
			return CONTROLLER_STATUS_READY;

		case CONTROLLER_STATUS_MOVING:
			if (in.mains == CONTROLLER_MAINS_OFF) {
				nulled = false;
				return CONTROLLER_STATUS_OFF;
			}
			if (!in.bus_moving) {
				if (in.mains != CONTROLLER_MAINS_MOTOR) {
					return CONTROLLER_STATUS_STANDBY;
				}
				if (!controller_direction_selected(in.dir)) {
					return CONTROLLER_STATUS_ON;
				}
				return CONTROLLER_STATUS_READY;
			}
			return CONTROLLER_STATUS_MOVING;

		case CONTROLLER_STATUS_SETUP:
			if (in.mains == CONTROLLER_MAINS_OFF) {
				nulled = false;
				return CONTROLLER_STATUS_OFF;
			}
			return CONTROLLER_STATUS_SETUP;

		default:
			nulled = false;
			return CONTROLLER_STATUS_OFF;
	}
}
