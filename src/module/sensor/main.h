/*
 * Loco-CAN sensor function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 */

/* MODULEALITY

 */

/* PARAMETERS

 */
#pragma once

#ifndef MODULE_SENSOR_H
#define MODULE_SENSOR_H

/* GLOBAL COMPONENTS*/
#include "../../config.h"


/* CORE COMPONENTS */
#include "../../core/can/can_com.h"
#include "../../can_protocol.h"

#define SENSOR_TYPE_ID 0x30
#define LOCO_MODULE_TYPE SENSOR_TYPE_ID
#define LOCO_MODULE_VERSION SENSOR_MODULE_VERSION


extern CAN_COM can;
extern CAN_MESSAGE can_message;


class MODULE_SENSOR {

	public:
		void begin(void);
		void update(CAN_MESSAGE message);

	private:


};

#endif