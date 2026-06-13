/*
 * Loco-CAN servo function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

// #include "../../config.h"
#include "main.h"

#ifdef MODULE_SERVO_CONFIG_H

void MODULE_SERVO::begin(void) {

	#ifdef DEBUG
		Serial.println("********************");
		Serial.println("start function/servo");
	#endif

}

void MODULE_SERVO::update(CAN_MESSAGE message) {

	can_message = message;
}

#endif