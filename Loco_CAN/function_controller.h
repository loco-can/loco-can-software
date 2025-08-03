#ifndef FUNCTION_CONTROLLER_H
#define FUNCTION_CONTROLLER_H

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

class FUNCTION_CONTROLLER {

public:
	void begin(void);
};


#endif