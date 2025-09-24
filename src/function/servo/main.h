/*
 * Loco-CAN servo function
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

#define FUNCTION_SERVO_H

extern CAN_COM can;
extern CAN_MESSAGE can_message;

/* GLOBAL COMPONENTS*/
#include "../../../config.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */
#include "../../core/can/can_com.h"
#include "../../core/servo/intelliServo.h"


class FUNCTION_SERVO {

	public:
		void begin(uint8_t func_id);
		void update(CAN_MESSAGE message);

	private:
		uint8_t _func_id;

};