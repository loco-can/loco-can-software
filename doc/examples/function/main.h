/*
 * Loco-CAN example function
 *
 * @author: 
 * @copyright: 
 * @lizence: GG0
 *
 */

/* FUNCTIONALITY
	Add a description of the functionality of this function.
 */

/* PARAMETERS
	Add a definition of the used parameters, that must be defined
	in the module h-file.
 */

#pragma once

#define FUNCTION_SENSOR_H

/* GLOBAL COMPONENTS*/
#include "../../../config.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */
#include "../../core/can/can_com.h"


class FUNCTION_{function_name} {

	public:
		void begin(uint8_t func_id);
		void update(CAN_MESSAGE message);

	private:
		uint8_t _func_id;

};