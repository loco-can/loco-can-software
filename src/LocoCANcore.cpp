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
	can.begin(CAN_BUS_SPEED, CAN_STATUS_LED);


	/* ********************************************************
	 * start all registered functions
	 ******************************************************* */
	#ifdef DEBUG
		Serial.println("*******************");
		Serial.println("starting functions");
	#endif

	// =========================
	// start function CONTROLLER
	#ifdef FUNCTION_CONTROLLER_H
		_controller.begin();
	#endif

	// =========================
	// start function GAUGE
	#ifdef FUNCTION_GAUGE_H
		_gauge.begin();
	#endif

	// =========================
	// start function MOTOR
	#ifdef FUNCTION_MOTOR_H
		_motor.begin();
	#endif

	// =========================
	// start function SWITCH
	#ifdef FUNCTION_SWITCH_H
		_switch.begin();
	#endif

	// =========================
	// start function SERVO
	#ifdef FUNCTION_SERVO_H
		_servo.begin();
	#endif

	// =========================
	// start function SERVO
	#ifdef FUNCTION_SENSOR_H
		_sensor.begin();
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

	/*
	 * update registered functions
	 */
	#ifdef FUNCTION_CONTROLLER_H
		_controller.update();
	#endif

	// =========================
	// start function GAUGE
	#ifdef FUNCTION_GAUGE_H
		_gauge.update();
	#endif

	// =========================
	// start function MOTOR
	#ifdef FUNCTION_MOTOR_H
		_motor.update();
	#endif

	// =========================
	// start function SWITCH
	#ifdef FUNCTION_SWITCH_H
		_switch.update();
	#endif

	// =========================
	// start function SERVO
	#ifdef FUNCTION_SERVO_H
		_servo.update();
	#endif

	// =========================
	// start function SERVO
	#ifdef FUNCTION_SENSOR_H
		_sensor.update();
	#endif


}