/*
 * Loco-CAN
 * 
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 * Version 3.x
 *
 */

// #include "../config.h"
#include "can_protocol.h"
#include "LocoCANcore.h"


CAN_COM can;


void LocoCANcore::begin(void) {

	#ifdef DEBUG
		Serial.println("*******************");
		Serial.println("* starting core");
	#endif

	/*
	 * create CAN class
	 * CAN_TX, CAN_TX and CAN_STATUS_LED are defined in the module settings h file
	 * CAN_BUS_SPEED is set in the can_protocol.h
	 */
	can.setPorts(CAN_RX, CAN_TX);
	can.set_alive(CAN_ALIVE_TIMEOUT);
	can.begin(CAN_BUS_SPEED, CAN_STATUS_LED); // start with one CAN LED


	/* ********************************************************
	 * start all registered functions
	 ******************************************************* */
	uint8_t func = 1;

	#ifdef DEBUG
		Serial.println("*******************");
		Serial.println("starting functions");
	#endif

	// =========================
	// start function CONTROLLER
	#ifdef FUNCTION_CONTROLLER_H
		_controller.begin(func++);
	#endif

	// =========================
	// start function GAUGE
	#ifdef FUNCTION_GAUGE_H
		_gauge.begin(func++);
	#endif

	// =========================
	// start function MOTOR
	#ifdef FUNCTION_MOTOR_H
		_motor.begin(func++);
	#endif

	// =========================
	// start function SWITCH
	#ifdef FUNCTION_SWITCH_H
		_switch.begin(func++);
	#endif

	// =========================
	// start function SERVO
	#ifdef FUNCTION_SERVO_H
		_servo.begin(func++);
	#endif

	// =========================
	// start function SERVO
	#ifdef FUNCTION_SENSOR_H
		_sensor.begin(func++);
	#endif

	#ifdef DEBUG
		Serial.println();
		Serial.println("*****************************");
		Serial.println("Loco-CAN started successfully");
		Serial.println("*****************************");
	#endif
	/* ***************************************************** */
}


void LocoCANcore::update(void) {

	CAN_MESSAGE message;

	message.uuid = 0;
	message.func = 0xFF;

	can.read(message);

	/*
	 * update registered functions
	 */
	#ifdef FUNCTION_CONTROLLER_H
		_controller.update(message);
	#endif

	// =========================
	// start function GAUGE
	#ifdef FUNCTION_GAUGE_H
		_gauge.update(message);
	#endif

	// =========================
	// start function MOTOR
	#ifdef FUNCTION_MOTOR_H
		_motor.update(message);
	#endif

	// =========================
	// start function SWITCH
	#ifdef FUNCTION_SWITCH_H
		_switch.update(message);
	#endif

	// =========================
	// start function SERVO
	#ifdef FUNCTION_SERVO_H
		_servo.update(message);
	#endif

	// =========================
	// start function SERVO
	#ifdef FUNCTION_SENSOR_H
		_sensor.update(message);
	#endif

	can.send();

}