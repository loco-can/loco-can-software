/*
 * Loco-CAN example function
 *
 * @author:
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../../config.h"
#include "main.h"


void FUNCTION_{function_name}::begin(uint8_t func_id) {

	_func_id = func_id;

	#ifdef DEBUG
		Serial.println("******************************");
		Serial.println("start function/{function_name}");
	#endif

}

void FUNCTION_SENSOR::update(CAN_MESSAGE message) {

	can_message = message;
}