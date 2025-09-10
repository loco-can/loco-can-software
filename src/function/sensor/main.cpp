/*
 * Loco-CAN sensor function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../../config.h"
#include "main.h"


void FUNCTION_SENSOR::begin(void) {

	#ifdef DEBUG
		Serial.println("*********************");
		Serial.println("start function/sensor");
	#endif

}

void FUNCTION_SENSOR::update(void) {

}