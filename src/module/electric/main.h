/*
 * Loco-CAN electric loco function
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

#ifndef MODULE_ELECTRIC_H
#define MODULE_ELECTRIC_H


/* GLOBAL COMPONENTS*/
#include "../../config.h"

#include "../../core/can/can_com.h"
#include "../../can_protocol.h"


/* CORE COMPONENTS */


extern CAN_COM can;
extern CAN_MESSAGE can_message;


class MODULE_ELECTRIC {

	public:
		void begin(void);
		void update(CAN_MESSAGE message);

	private:

};

#endif