/*
 * Loco-CAN sensor function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../../config.h"
#include "main.h"


void FUNCTION_SENSOR::begin(uint8_t func_id) {

	_func_id = func_id;

	#ifdef DEBUG
		Serial.println("*********************");
		Serial.println("start function/sensor");
	#endif

}

void FUNCTION_SENSOR::update(CAN_MESSAGE message) {

	can_message = message;
}