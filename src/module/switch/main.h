/*
 * Loco-CAN switch function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2026
 * @lizence: GG0
 *
 */

/* FUNCTIONALITY
	Six outputs. Each output is mapped to one bit of a CAN message.
	The map is a module setting and is stored in EEPROM.

	Defaults:
		1 light_low_front    low beam at the front, on when going forward
		2 light_low_back     low beam at the back, on when reversing
		3 light_back_front   tail lamp at the front, on when reversing
		4 light_back_back    tail lamp at the back, on when going forward
		5 horn_low           SIGNAL_LOW
		6 horn_high          SIGNAL_HIGH

	Direction is CONTROL_DIR_FLAG in the drive frame (0 = forward).
	Headlamps light on the leading end. Tail lamps light on the
	trailing end.
 */

/* PARAMETERS
	SWITCH_PORT_COUNT
	LIGHT1 .. LIGHT6    output pins, index 0 = LIGHT1
 */
#pragma once

#ifndef MODULE_SWITCH_H
#define MODULE_SWITCH_H

/* GLOBAL COMPONENTS*/
#include "../../config.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */
#include "../../core/can/can_com.h"
#include "../../core/timeout/intellitimeout.h"

#include "params.h"


extern CAN_COM can;
extern CAN_MESSAGE can_message;


class MODULE_SWITCH {

	public:
		void begin(void);
		void update(CAN_MESSAGE message);

	private:
		void _load_params(void);
		void _save_params(void);
		void _handle_setup(CAN_MESSAGE message);
		void _write_outputs(void);

		SWITCH_PARAMS _params;
		SWITCH_BUS _bus;
		INTELLITIMEOUT _bus_timeout;

};

#endif
