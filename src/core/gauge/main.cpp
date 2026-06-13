/*
 * Loco-CAN gauge function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../config.h"
#include "main.h"


void GAUGE::begin(void) {

	#ifdef DEBUG
		Serial.println("********************");
		Serial.println("start function/gauge");
	#endif


    // **********************************
    // GAUGES

	// BATTERY VOLTAGE GAUGE
    #ifdef CONTROLLER_BATTERY_VOLTAGE_PORT
		#ifdef DEBUG
			Serial.print("> init battery voltage gauge on port ");
			Serial.println(CONTROLLER_BATTERY_VOLTAGE_PORT);
		#endif
		_battery_voltage_gauge.begin(CONTROLLER_BATTERY_VOLTAGE_PORT);
	#endif

	// MOTOR VOLTAGE GAUGE
    #ifdef CONTROLLER_MOTOR_VOLTAGE_PORT
		#ifdef DEBUG
			Serial.print("> init motor voltage gauge on port ");
			Serial.println(CONTROLLER_MOTOR_VOLTAGE_PORT);
		#endif
		_motor_voltage_gauge.begin(CONTROLLER_MOTOR_VOLTAGE_PORT);
	#endif

	// BATTERY CURRENT GAUGE
    #ifdef CONTROLLER_BATTERY_CURRENT_PORT
		#ifdef DEBUG
			Serial.print("> init battery current gauge on port ");
			Serial.println(CONTROLLER_BATTERY_CURRENT_PORT);
		#endif
		_battery_current_gauge.begin(CONTROLLER_BATTERY_CURRENT_PORT);
	#endif

	// MOTOR CURRENT GAUGE
    #ifdef CONTROLLER_MOTOR_CURRENT_PORT
		#ifdef DEBUG
			Serial.print("> init motor current gauge on port ");
			Serial.println(CONTROLLER_MOTOR_CURRENT_PORT);
		#endif
		_motor_current_gauge.begin(CONTROLLER_MOTOR_CURRENT_PORT);
	#endif

}

void GAUGE::update(CAN_MESSAGE message) {

	can_message = message;
}