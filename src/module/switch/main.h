/*
 * Loco-CAN switch function
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

#ifndef MODULE_SWITCH_H
#define MODULE_SWITCH_H

/* GLOBAL COMPONENTS*/
#include "../../config.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */
#include "../../core/can/can_com.h"


extern CAN_COM can;
extern CAN_MESSAGE can_message;


class MODULE_SWITCH {

	public:
		void begin(void);
		void update(CAN_MESSAGE message);

	private:


};

#endif