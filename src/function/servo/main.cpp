/*
 * Loco-CAN servo function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../../config.h"
#include "main.h"


void FUNCTION_SERVO::begin(uint8_t func_id) {

	_func_id = func_id;

	#ifdef DEBUG
		Serial.println("********************");
		Serial.println("start function/servo");
	#endif

}

void FUNCTION_SERVO::update(CAN_MESSAGE message) {

	_message = message;
}