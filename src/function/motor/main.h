/*
 * Loco-CAN motor function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 */

/* FUNCTIONALITY

 */

/* PARAMETERS

 */

#pragma once

#define FUNCTION_MOTOR_H

extern CAN_COM can;
extern CAN_MESSAGE can_message;

/* GLOBAL COMPONENTS*/
#include "../../../config.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */
#include "../../core/can/can_com.h"


class FUNCTION_MOTOR {

	public:
		void begin(uint8_t func_id);
		void update(CAN_MESSAGE message);

	private:
		uint8_t _func_id;

};