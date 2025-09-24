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
		#ifdef FUNCTION_CONTROLLER_H
			FUNCTION_CONTROLLER _controller;
		#endif

		#ifdef FUNCTION_GAUGE_H
			FUNCTION_GAUGE _gauge;
		#endif

		#ifdef FUNCTION_MOTOR_H
			FUNCTION_MOTOR _motor;
		#endif

		#ifdef FUNCTION_SWITCH_H
			FUNCTION_SWITCH _switch;
		#endif

		#ifdef FUNCTION_SERVO_H
			FUNCTION_SERVO _servo;
		#endif

		#ifdef FUNCTION_SENSOR_H
			FUNCTION_SENSOR _sensor;
		#endif
};