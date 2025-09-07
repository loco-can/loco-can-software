/*
 * Loco-CAN gauge function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../../config.h"
#include "main.h"


void FUNCTION_GAUGE::begin(CAN_COM can) {

	#ifdef DEBUG
		Serial.println("********************");
		Serial.println("start function/gauge");
	#endif

	_can = can;
}

void FUNCTION_GAUGE::update(void) {

}