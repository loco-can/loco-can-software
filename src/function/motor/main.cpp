/*
 * Loco-CAN motor function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../../config.h"
#include "main.h"


void FUNCTION_MOTOR::begin(uint8_t func_id) {

	_func_id = func_id;

	#ifdef DEBUG
		Serial.println("********************");
		Serial.println("start function/motor");
	#endif

}

void FUNCTION_MOTOR::update(CAN_MESSAGE message) {

	can_message = message;
}