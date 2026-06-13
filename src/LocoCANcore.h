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

#pragma once

#ifndef LOCOCANCORE_H
#define LOCOCANCORE_H

#include "config.h"
#include "core/can/can_com.h"
#include "core/ping/modulePing.h"


class LocoCANcore {

	public:
		void begin(void);
		void update(void);

	private:

		MODULE_PING _ping;

		/*
		 * define all functions that are included in the module/{module_name}.h file
		 */
		// train controller
		#ifdef MODULE_CONTROLLER_H
			MODULE_CONTROLLER _module;
		#endif

		// gauge to display measured values
		#ifdef MODULE_GAUGE_H
			MODULE_GAUGE _module;
		#endif

		// remote controlled switches
		#ifdef MODULE_SWITCH_H
			MODULE_SWITCH _module;
		#endif

		// servo motor driver
		#ifdef MODULE_SERVO_H
			MODULE_SERVO _module;
		#endif

		// sensor functions
		#ifdef MODULE_SENSOR_H
			MODULE_SENSOR _module;
		#endif

		// electric driven locomotive
		#ifdef MODULE_ELECTRIC_H
			MODULE_ELECTRIC _module;
		#endif

		// combustion motor driven locomotive
		#ifdef MODULE_COMBUSTION_H
			MODULE_COMBUSTION _module;
		#endif
};

#endif