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

/* FUNCTIONALITY
	In a train setup many controllers and vehicle modules can be connected to the
	same bus. Only one controller can be active at the same time.

	When turning on the main switch, the controller checks for other controller
	heartbeats. If none is found, the own heartbeat is switched on and the mains
	status goes to on. The pairing-ID stays saved, even if mains is switched of
	or the system is powerded down.

	PAIRING
	A controller must be paired to one vehicle module. If only one is found, the
	UUID of the corresponding vehicle is stored as pairing-ID in the controller.
	From now on, the controller uses this pairing-ID in its own CAN-message-ID.
	Only the measurements of the paired vehicle are sent to the BUS and displayed
	on all controllers.

	If more than one vehicle is found while switching on, the pairing depends on
	the situation.

	1.	A before paired vehicle is found: The pairing is valid and used.
	2.	No paired vehicle, is found: The first received UUID is used to pair
		The paired vehicle can now be selected using the loco-setup procedure.

	LOCO-SETUP
	Pairing and the forward direction of each vehicle can be set using this
	sequence.

	1.	Press the horn button
	2.	Switch on the mains switch -> the controller is now in loco-setup moode
	3.	A loco-setup message is sent to the bus and all vehiclem modules change to
		loco-setup mode.
	4.	The paired locomotive will blink with all light on the forward end.
	
	A short push on the horn button switches to the next locomotive and paires with it.
	Switching the direction lever changes the forward direction of the locomotive.

	The setup ist ended by switching off mains. All settings are saved automatically.
 */

/* STATUS BITS
	mains 		mains switch status
	drive 		the drive mechanism is activated
	dir 		direction (0=forward, 1=reverse)
	reverse 	the direction logic is reversed
	paired 		the controller and vehicle are paired
	stop 		the train is not moving
	ready 		the system is ready to start
	error 		an error occured, the train is stopped

	The status of the drive message and the returned vehicle status from the paired
	vehicle are compaired and has to be equal for correct function. If more than one
	vehicle is connected, the mains, drive, ready and error values has to match.
*/


/* DRIVE MESSAGE

	Byte  |    7    |    6    |    5    |    4    |    3    |    2    |    1    |    0    |
	---------------------------------------------------------------------------------------
	  0   | error	| ready	  | stop 	| paired  | reverse	| dir 	  | drive 	| mains   |
	  1   | uuid-15 | uuid-14 | uuid-13 | uuid-12 |         |         | drive-9 | drive-8 |
	  2   | drive-7 | drive-6 | drive-5 | drive-4 | drive-3 | drive-2 | drive-1 | drive-0 |
	  3   | uuid-11 | uuid-10 |  uuid-9 | uuid-8  |         |         | power-9 | power-8 |
	  4   | power-7 | power-6 | power-5 | power-4 | power-3 | power-2 | power-1 | power-0 |
	  5   |			|         |         |         | 	    |         | break-9 | break-8 |
	  6   | break-7 | break-6 | break-5 | break-4 | break-3 | break-2 | break-1 | break-0 |
	  7   | uuid-7  | uuid-6  | uuid-5  | uuid-4  | uuid-3  | uuid-2  | uuid-1  | uuid-0  |
 */

/* VEHICLE STATUS MESSAGE

	Byte  |    7    |    6    |    5    |    4    |    3    |    2    |    1    |    0    |
	---------------------------------------------------------------------------------------
	  0   | error	| ready	  | stop 	| paired  | reverse	| dir 	  | drive 	| mains   |
 */

/* PARAMETERS
	The mandatory parameters sets the basic settings and the hardware mapping.
	Additional parameters can select additional functions like supported gauges
	for voltage, current or speed.

	(optional parameters are in [])

	CONTROLLER_ANALOG_RESOLUTION => resolution for analog inputs (depends on platform)

	CONTROLLER_MAINS_PORT => port for mains switch
	CONTROLLER_DIR_PORT => port for direction switch
	[ CONTROLLER_HORN_PORT => port for horn button ]
	[ CONTROLLER_HORN2_PORT => port for second horn button ]
	[ CONTROLLER_LIGHT_PORT => port for light switch ]
	[ CONTROLLER_LIGHT2_PORT => port for second light switch ]
	[ CONTROLLER_INSTRUMENT_LIGHT_PORT => port for instrument light switch ]

	[ CONTROLLER_BATTERY_VOLTAGE_PORT => port for battery voltage gauge ]
	[ CONTROLLER_MOTOR_VOLTAGE_PORT => port for motor voltage gauge ]
	[ CONTROLLER_CURRENT_PORT => port for main current port ]
	[ CONTROLLER_MOTOR_CURRENT_PORT => port for motor current port ]

	CONTROLLER_DRIVE_MODE
		SINGLE => drive and break with a single controller
		DUAL => two controllers for drive and break
		POWER => two controllers for speed and power

	CONTROLLER_DRIVE_PORT => port for drive [drive/break] pot
	[ CONTROLLER_BREAK_PORT => port for break pot ]
	[ CONTROLLER_POWER_PORT => port for power pot ]

	CONTROLER_STATUS_MODE
		LED => use dual LED for status display
			CONTROLLER_STATUS_RED_PORT => port for status LED
			CONTROLLER_STATUS_GREEN_PORT => port for status LED

		[ OLED => use OLED display ]
			[ CONTROLLER_OLED_TYPE ]
				{list of supported OLED displays}
 */

#pragma once

#define FUNCTION_CONTROLLER_H

/* GLOBAL COMPONENTS*/
#include "../../../config.h"
#include "../../core/can/can_com.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */
#include "../../core/analogSwitch/analogSwitch.h"
#include "../../core/button/intelliButton.h"
#include "../../core/servo/intelliServo.h"
#include "../../core/timeout/intellitimeout.h"
#include "../../core/flags/flags.h"
#include "../../core/handshake/handshake.h"


/* local classes */


/*
	Flags that influence the behavior of the controller-vehicle communication:

		0 	blocked		another controller is active, this is blocked
		1 	connected	the controller is connected to the vehicle
		2 	paired		the controller is paired with the vehicle
 */

#define CONTROLLER_BLOCKED		0
#define CONTROLLER_CONNECTED	1
#define CONTROLLER_PAIRED		2


class FUNCTION_CONTROLLER {

	public:
		void begin(uint8_t func_id);
		void update(CAN_MESSAGE message);

	private:
		// HANDSHAKE _handshake;

		uint8_t _func_id;
		CAN_MESSAGE _message;

		// the controller status
		uint8_t _status;

		// additional flags
		FLAGS _controller_flags;


		INTELLITIMEOUT _drive_time;

		ANALOGSWITCH _mains_switch;
		ANALOGSWITCH _dir_switch;
		INTELLIBUTTON _horn_switch;
		INTELLIBUTTON _horn2_switch;

		ANALOGSWITCH _light_switch;
		INTELLIBUTTON _light2_switch;
		INTELLIBUTTON _instrument_switch;
};