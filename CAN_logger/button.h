/*
 * BUTTON class
 *
 * The class offers a simple way to use up to two buttons
 * A state is returned, when the button is released, the
 * program is not stopped in this time.
 *
 * The status returned represents the state of the pushed
 * buttons:
 *		BUTTON_OFF	no button is presse
 *		BUTTON_UP	button 1 is pressed
 *		BUTTON_DOWN	button 2 is pressed
 *		BUTTON_BOTH	both buttons are pressed simulaniously
 * The length method returns the time the buttons where pressed
 * in milliseconds. Calling this method with a time in milliseconds
 * returns boolean true, if the pressed time is greater or equal than
 * the parameter. 
 */


#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

#define BUTTON_DELAY 100

#define BUTTON_OFF 0
#define BUTTON_UP 1
#define BUTTON_DOWN 2
#define BUTTON_BOTH 3


class BUTTON {

	public:
		BUTTON();
		void begin(int8_t port);
		void begin(int8_t port1, int8_t port2);
		uint8_t get(void);
		bool pressed(void);
		uint32_t length(void);
		bool length(uint32_t);

	private:
		int8_t _port1;
		int8_t _port2;
		long _timeout;
		uint8_t _status;
		uint32_t _pushed;
		uint32_t _length;
};


#endif