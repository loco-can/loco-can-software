/*
 * Loco-CAN gauge function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 */

/* FUNCTIONALITY

 */

/* PARAMETERS

 */
#pragma once

#ifndef GAUGE_H
#define GAUGE_H


/* GLOBAL COMPONENTS*/
#include "../../config.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */
#include "../../core/can/can_com.h"
#include "../../core/servo/intelliServo.h"

extern CAN_COM can;
extern CAN_MESSAGE can_message;

class GAUGE {

	public:
		void begin(void);
		void update(CAN_MESSAGE message);

	private:

		INTELLISERVO _battery_voltage_gauge;
		INTELLISERVO _motor_voltage_gauge;
		INTELLISERVO _battery_current_gauge;
		INTELLISERVO _motor_current_gauge;
};

#endif