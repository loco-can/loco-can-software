/*
 * Loco-CAN gauge function
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

#define FUNCTION_GAUGE_H

/* GLOBAL COMPONENTS*/
#include "../../../config.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */
#include "../../core/can/can_com.h"
#include "../../core/servo/intelliServo.h"


class FUNCTION_GAUGE {

	public:
		void begin(CAN_COM can);
		void update(void);

	private:
		CAN_COM _can;
		CAN_MESSAGE _message;

};