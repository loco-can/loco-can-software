/*
 * Loco-CAN switch function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

// #include "../../config.h"
#include "main.h"

#ifdef MODULE_SWITCH_CONFIG_H

void MODULE_SWITCH::begin(void) {

	#ifdef DEBUG
		Serial.println("*********************");
		Serial.println("start function/switch");
	#endif

}

void MODULE_SWITCH::update(CAN_MESSAGE message) {

	can_message = message;
}

#endif