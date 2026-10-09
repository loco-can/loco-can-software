/*
 * Loco-CAN electric loco function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 * The electric module is the vehicle side of a controller. It reads the
 * drive, heartbeat, emergency and loco-setup messages, measures the motor
 * and battery voltages, and drives the selected motor-controller plugin.
 *
 * Plugin type, battery count and PWM mode are stored in EEPROM and edited
 * over the CAN settings protocol.
 */

/* MODULEALITY
	Listen to the active controller.
	Publish vehicle status, motor voltage and the configured battery voltages.
	Direct plugin: drive PWM, brake PWM, forward relay, reverse relay,
	optional power-line relay. Direction changes wait until the motor
	voltage is under the stored minimum.
	4QD and Curtis plugins use the same pins with that controller's
	enable, reverse and throttle rules.
 */

/* PARAMETERS
	DRIVE_PWM, DRIVE_BREAK, DRIVE_FORW, DRIVE_REV
	DRIVE_MOTOR_VOLTAGE_PLUS, DRIVE_MOTOR_VOLTAGE_MINUS
	[ DRIVE_POWER ]
	ELECTRIC_BATT_0 .. ELECTRIC_BATT_2
 */
#pragma once

#ifndef MODULE_ELECTRIC_H
#define MODULE_ELECTRIC_H


/* GLOBAL COMPONENTS*/
#include "../../config.h"

#include "../../core/can/can_com.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */
#include "../../core/measure/measure.h"
#include "../../core/timeout/intellitimeout.h"

#include "plugin.h"
#include "sensors.h"
#include "settings.h"

#define LOCO_MODULE_TYPE ELECTRIC_TYPE_ID
#define LOCO_MODULE_VERSION ELECTRIC_MODULE_VERSION


extern CAN_COM can;
extern CAN_MESSAGE can_message;


class MODULE_ELECTRIC {

	public:
		void begin(void);
		void update(CAN_MESSAGE message);

	private:

		void _load_settings(void);
		void _save_settings(void);
		void _handle_settings(CAN_MESSAGE message);
		void _handle_can(CAN_MESSAGE message);
		void _read_sensors(void);
		void _write_outputs(const ELECTRIC_OUTPUT &output);
		void _send_status(const ELECTRIC_STATUS_BITS &bits);
		void _send_sensors(void);

		ELECTRIC_SETTINGS _settings;
		ELECTRIC_PLUGIN_STATE _plugin_state;
		ELECTRIC_COMMAND _command;
		uint8_t _applied_plugin;
		uint8_t _applied_mode;

		bool _seen_drive;
		bool _link_fault;
		bool _emergency;
		bool _setup_active;
		bool _setup_selected;
		bool _setup_dir_level;
		bool _gap_waiting;

		int32_t _motor_voltage;
		uint16_t _battery[ELECTRIC_BATT_MAX];

		MEASURE _motor;
		MEASURE _battery_input[ELECTRIC_BATT_MAX];

		INTELLITIMEOUT _drive_timeout;
		INTELLITIMEOUT _direction_gap;
		INTELLITIMEOUT _setup_timeout;
		INTELLITIMEOUT _status_time;
		INTELLITIMEOUT _sensor_time;

		CAN_MESSAGE _tx;
};

#endif
