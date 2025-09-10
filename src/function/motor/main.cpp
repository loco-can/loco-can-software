/*
 * Loco-CAN motor function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../../config.h"
#include "main.h"


void FUNCTION_MOTOR::begin(void) {

	#ifdef DEBUG
		Serial.println("********************");
		Serial.println("start function/motor");
	#endif

}

void FUNCTION_MOTOR::update(void) {

}