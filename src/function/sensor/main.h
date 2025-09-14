/*
 * Loco-CAN sensor function
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

#define FUNCTION_SENSOR_H

/* GLOBAL COMPONENTS*/
#include "../../../config.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */
#include "../../core/can/can_com.h"


class FUNCTION_SENSOR {

	public:
		void begin(uint8_t func_id);
		void update(CAN_MESSAGE message);

	private:
		uint8_t _func_id;
		CAN_MESSAGE _message;

};