/*
 * Loco-CAN sensor function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

// #include "../../config.h"
#include "main.h"

#ifdef MODULE_SENSOR_CONFIG_H

void MODULE_SENSOR::begin(void) {

	#ifdef DEBUG
		Serial.println("*********************");
		Serial.println("start function/sensor");
	#endif

}

void MODULE_SENSOR::update(CAN_MESSAGE message) {

	can_message = message;
}

#endif