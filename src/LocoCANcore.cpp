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

// #include "core/can/can_com.h"

// CAN_COM can_com(CAN_RX, CAN_TX);


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
	_can.setPorts(CAN_RX, CAN_TX);
	_can.set_alive(CAN_ALIVE_TIMEOUT);
	_can.begin(CAN_BUS_SPEED, CAN_STATUS_LED);


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
		_controller.begin(_can);
	#endif

	// =========================
	// start function GAUGE
	#ifdef FUNCTION_GAUGE_H
		_gauge.begin(_can);
	#endif

	// =========================
	// start function MOTOR
	#ifdef FUNCTION_MOTOR_H
		_motor.begin(_can);
	#endif

	// =========================
	// start function SWITCH
	#ifdef FUNCTION_SWITCH_H
		_switch.begin(_can);
	#endif

	// =========================
	// start function SERVO
	#ifdef FUNCTION_SERVO_H
		_servo.begin(_can);
	#endif

	// =========================
	// start function SERVO
	#ifdef FUNCTION_SENSOR_H
		_sensor.begin(_can);
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
	// #ifdef FUNCTION_CONTROLLER
	// 	_controller.update();
	// #endif



}