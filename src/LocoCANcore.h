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

#include "../config.h"
#include "core/can/can_com.h"


class LocoCANcore {

	public:
		void begin(void);
		void update(void);

	private:

		/*
		 * define all functions that are included in the module/{module_name}.h file
		 */

		// train controller
		#ifdef FUNCTION_CONTROLLER_H
			FUNCTION_CONTROLLER _controller;
		#endif

		// gauge to display measured values
		#ifdef FUNCTION_GAUGE_H
			FUNCTION_GAUGE _gauge;
		#endif

		// remote controlled switches
		#ifdef FUNCTION_SWITCH_H
			FUNCTION_SWITCH _switch;
		#endif

		// servo motor driver
		#ifdef FUNCTION_SERVO_H
			FUNCTION_SERVO _servo;
		#endif

		// sensor functions
		#ifdef FUNCTION_SENSOR_H
			FUNCTION_SENSOR _sensor;
		#endif

		// electric driven locomotive
		#ifdef FUNCTION_ELECTRIC_H
			FUNCTION_ELECTRIC _electric;
		#endif

		// combustion motor driven locomotive
		#ifdef FUNCTION_COMBUSTION_H
			FUNCTION_COMBUSTION _combustion;
		#endif
};
