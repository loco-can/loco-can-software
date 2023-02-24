#ifndef MODULE_H
#define MODULE_H

#include "config.h"
#include "motor.h"
#include "can_com.h"

class MODULE {

	public:
		void begin(CAN_COM*);
		void update(void);

	private:
		CAN_COM* _can_com;
		MOTOR _servo[4];

};

#endif