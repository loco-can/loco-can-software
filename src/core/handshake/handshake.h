/*
 * Loco-CAN controller function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 * The controller function is the interface between controller IOs for speed,
 * direction, switches i.e. and the CAN bus. Controller interact with vehicle
 * functions.
 */

#pragma once

/* FUNCTIONALITY
 * The handshake class is used to coordinate the critical communication
 * between two modules.
 *
 * It is started on the two functions as master and slave.
 */


/* CONTROLLER - VEHICLE STATUS
	Both the status of controllers and vehicles are represented by a value
	between 0 and 7.

	Controller status
 	-----------------
		0 	off			the controller mains switch is off
		1 	on 			the controller mains switch is on
		2 	standby		no direction is selected and the drive is off
		3 	ready		direction selected, ready to start
		4	moving		the train is moving
		5	 ---		not used
		6	setup		the controller is in setup mode
		7	error		an error occured in the controller or vehicle


	Vehicle status
	--------------
		0 	off			the vehicles main switech is of
		1 	on 			the vehicle is switched on
		2	 ---		not used
		3 	ready 		the vehicle is ready to start
		3 	moving		the vehicle is moving
		5	 ---		not used
		6	setup		the vehicle is in setup mode
		7	error		an error occured in the vehicle
 */

#define HANDSHAKE_STATUS_OFF		0
#define HANDSHAKE_STATUS_ON			1
#define HANDSHAKE_STATUS_STANDBY	2
#define HANDSHAKE_STATUS_READY		3
#define HANDSHAKE_STATUS_MOVING		4
#define HANDSHAKE_STATUS_SETUP		6
#define HANDSHAKE_STATUS_ERROR		7


/* STATUS MESSAGE

	Byte  |    7    |    6    |    5    |    4    |    3    |    2    |    1    |    0    |
	---------------------------------------------------------------------------------------
	  0   | error	| ready	  | stop 	| paired  | reverse	| dir 	  | drive 	| mains   |
 */


/* COMPARE MASK
	The drive and vehicle status bytes are compared to ensure correct communication
 */
#define STATUS_COMPARE_MASK 		0b11011011


// #include "../../../config.h"
#include "../../core/can/can_com.h"
#include "../../core/timeout/intellitimeout.h"
#include "../../core/flags/flags.h"


extern CAN_COM can;


class HANDSHAKE {

	public:

		// start handshake class with connection time out
		void begin(uint16_t timeout);

		// set stati from message or status byte
		// for master use update_master(uint8_t) and update_slave(CAN_MESSAGE)
		// for slave use update_slave(uint8_t) and update_master(CAN_MESSAGE)
		void update_master(CAN_MESSAGE message);
		void update_master(FLAGS status);
		void update_master(uint8_t status);

		void update_slave(CAN_MESSAGE message);
		void update_slave(FLAGS status);
		void update_slave(uint8_t status);

		// get master or slave status
		FLAGS master_status(void);
		FLAGS slave_status(void);

	private:
		void send_master(CAN_MESSAGE message);
		void send_slave(CAN_MESSAGE message);

		// connection timeout
		INTELLITIMEOUT _timeout;

		FLAGS _master_status;
		FLAGS _slave_status;
};