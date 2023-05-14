/*
 * LOCO-CAN Controller Module header file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */


#ifndef MODULE_H
#define MODULE_H


#include <Servo.h>

#include "config.h"

#include "can_com.h"
#include "flags.h"
#include "intelliLed.h"
#include "intelliTimeout.h"
#include "analogSwitch.h"
#include "intelliButton.h"
#include "meter.h"
#include "vehicles.h"


// #include "settings.h"

#define CONTROLLER_STATUS_LOCKED 0
#define CONTROLLER_STATUS_OFF 1

#define CONTROLLER_STATUS_NOT_NULLED 2

#define CONTROLLER_STATUS_STANDBY 3
#define CONTROLLER_STATUS_READY 4
#define CONTROLLER_STATUS_MOVING 5

#define CONTROLLER_STATUS_SETUP 6


class MODULE {

	public:
		void begin(void);
		void update(void);

	private:
		void _initialize(void);
		void _selftest(void);

		void _receive(CAN_MESSAGE);

		void _mains(void);
		void _dir(void);
		void _light(void);
		void _horn(void);

		uint8_t _set_status(CAN_MESSAGE);
		void _set_status_led(void);

		void _drive_break(void);
		void _send(void);

		bool _activate(void); // activate controller > returns new state

		bool _active;
		bool _nulled;
		
		uint16_t _value;

		uint16_t _drive_val;
		uint16_t _break_val;
		uint16_t _power_val;

		bool _collision; // drive message collision

		ANALOGSWITCH _mains_switch;
		ANALOGSWITCH _dir_switch;
		ANALOGSWITCH _light_switch;

		INTELLIBUTTON _signal;
		INTELLIBUTTON _signal_1;

		#ifdef ANALOG_LIGHT_1_SWITCH
			ANALOGSWITCH _light_1_switch;
			FLAGS _lights_1;
		#endif

		uint8_t _controller_status;
		FLAGS _status;
		FLAGS _switches;
		FLAGS _switches_1;
		FLAGS _lights;

		FLAGS _light_status; //can light status

		// measurement
		MEASURE_CALCULATE _voltage;
		METER _meter_volt;

		#ifdef METER_AMP
			MEASURE_CALCULATE _current;
			METER _meter_amp;
		#endif

		#ifdef METER_MOTOR_VOLT
			MEASURE_CALCULATE _voltage_motor;
			METER _meter_motor;
		#endif

		#ifdef METER_SPEED
			MEASURE_CALCULATE _speed;
			METER _meter_speed;
		#endif

		VEHICLES _vehicles;

		INTELLILED _status_led;

		INTELLITIMEOUT _timeout;
		INTELLITIMEOUT _collision_timeout;
		INTELLITIMEOUT _heartbeat;

};


#endif