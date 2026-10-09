/*
 * Loco-CAN servo function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2026
 * @lizence: GG0
 *
 */

/* FUNCTIONALITY
	Four RC servo outputs. Each output follows analog data from a CAN
	message. The map is a module setting and is stored in EEPROM.

	Defaults:
		1 drive throttle (10-bit in CAN_ID_DRIVE)
		2 brake
		3 power
		4 unmapped

	Sources: drive, brake, power, direction, speed, tacho, module /
	motor / battery current, main / motor / battery voltage.
	Bit 7 of a map byte inverts the 0..1023 value.

	A missing bus or an emergency frame parks mapped servos at 0.
 */

/* PARAMETERS
	SERVO_1 .. SERVO_4  output pins, index 0 = SERVO_1
 */
#pragma once

#ifndef MODULE_SERVO_H
#define MODULE_SERVO_H


/* GLOBAL COMPONENTS*/
#include "../../config.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */
#include "../../core/can/can_com.h"
#include "../../core/timeout/intellitimeout.h"

#include "params.h"

#define LOCO_MODULE_TYPE SERVO_TYPE_ID
#define LOCO_MODULE_VERSION SERVO_MODULE_VERSION


extern CAN_COM can;
extern CAN_MESSAGE can_message;


class MODULE_SERVO {

	public:
		void begin(void);
		void update(CAN_MESSAGE message);

	private:
		void _load_params(void);
		void _save_params(void);
		void _handle_setup(CAN_MESSAGE message);
		void _write_outputs(void);

		SERVO_PARAMS _params;
		SERVO_BUS _bus;
		INTELLITIMEOUT _bus_timeout;
};

#endif
