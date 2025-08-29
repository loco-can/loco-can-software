/*
 * LOCO-CAN DRIVE header file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */

/*
 * MODULE_DRIVE class
 */


#ifndef MODULE_DRIVE_H
#define MODULE_DRIVE_H


#include "config.h"

// #include "can_com.h"
// #include "can_protocol.h"

// #include "intelliLed.h"
// #include "pwm_ramp.h"
// #include "flags.h"

// extern CAN_COM can_com;


class MODULE {

	public:
		void begin(void);
		void update(void);

		void stop(void);
		
		void forw(void);
		void rev(void);

		void drive(void);
		void brk(void);

		void set_pwm(uint16_t);

	private:
		void ledc_init(void);
};


#endif