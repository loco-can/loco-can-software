/*
 * LOCO-CAN light Module configuration
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */

#ifndef MODULE_H
#define MODULE_H


#include "config.h"

#include "measure.h"
#include "settings.h"
#include "flags.h"
#include "intellitimeout.h"
#include "intelliled.h"
#include "intelliservo.h"


class MODULE {

	public:
		void begin(void);
		void update(void);
	
	private:

		bool send(uint8_t*, uint8_t, long);

		uint8_t _i;
		
		SETTINGS _settings;
		FLAGS _status;
		FLAGS _switches;

		SIMPLETIMEOUT _send_timeout;

		INTELLILED _leds[2];
		INTELLISERVO _servos[2];
};

#endif