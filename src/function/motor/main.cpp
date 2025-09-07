/*
 * Loco-CAN motor function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../../config.h"
#include "main.h"


void FUNCTION_MOTOR::begin(CAN_COM can) {

	#ifdef DEBUG
		Serial.println("********************");
		Serial.println("start function/motor");
	#endif

	_can = can;
}

void FUNCTION_MOTOR::update(void) {

}