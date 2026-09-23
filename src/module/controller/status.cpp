/*
 * Loco-CAN controller status transitions
 */

#include "status.h"


uint8_t controller_next_status(uint8_t status, const CONTROLLER_INPUTS &in, bool &nulled) {

	/*
	 * A vehicle error, CAN emergency or the local emergency brake
	 * drops every driving state back to off. Setup is included so a
	 * fault cannot leave the locomotives in setup mode.
	 */
	if (in.system_error && controller_is_commanding(status)) {
		nulled = false;
		return CONTROLLER_STATUS_OFF;
	}

	switch (status) {

		case CONTROLLER_STATUS_OFF:
			if (in.mains == CONTROLLER_MAINS_OFF) {
				nulled = false;
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
			if (in.dir != CONTROLLER_DIR_NEUTRAL) {
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
			if (in.dir != CONTROLLER_DIR_NEUTRAL) {
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
			if (in.dir == CONTROLLER_DIR_NEUTRAL) {
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
				if (in.dir == CONTROLLER_DIR_NEUTRAL) {
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
