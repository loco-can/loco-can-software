/*
 * Loco-CAN servo function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../../config.h"
#include "main.h"


void FUNCTION_SERVO::begin(CAN_COM can) {

	#ifdef DEBUG
		Serial.println("********************");
		Serial.println("start function/servo");
	#endif

	_can = can;
}

void FUNCTION_SERVO::update(void) {

}