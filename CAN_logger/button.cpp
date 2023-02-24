#include "button.h"

BUTTON::BUTTON() {
}


// start with one port
void BUTTON::begin(int8_t port) {
	begin(port, -1);
}


// start with two buttons
void BUTTON::begin(int8_t port1, int8_t port2) {

	_port1 = port1;
	_port2 = port2;
	_status = 0;
	_length = 0;

	pinMode(_port1, INPUT_PULLUP);

	// set second port
	if (_port2 != -1) {
		pinMode(_port2, INPUT_PULLUP);
	}
}


// get button state
uint8_t BUTTON::get(void) {

	uint8_t ret = BUTTON_OFF;
	bool b1;
	bool b2 = false;

	b1 = !digitalRead(_port1);

	if (_port2 != -1) {
		b2 = !digitalRead(_port2);
	}


	// button pressed
	if (b1 || b2) {

		// save push time at start
		if (_status == BUTTON_OFF) {
			_pushed = millis();
		}

		// delay for detect both buttons
		delay(BUTTON_DELAY);

		// get buttons again
		b1 = !digitalRead(_port1);
		b2 = !digitalRead(_port2);

		// check buttons
		if (b1 && b2) {
			_status = BUTTON_BOTH;
		}

		if (b1 && !b2) {
			_status = BUTTON_UP;
		}

		if (!b1 && b2) {
			_status = BUTTON_DOWN;
		}
	}

	// buttons released
	else {
		ret = _status;
		_status = BUTTON_OFF;

		// save push time
		_length = millis() - _pushed;
	}

	return ret;
}


// check if button is pressed
bool BUTTON::pressed(void) {

	if (!digitalRead(_port1) || !digitalRead(_port2)) {
		return true;
	}

	return false;
}


// get last push time
uint32_t BUTTON::length(void) {
	return _length;
}


// check if button was presst for minimal time
bool BUTTON::length(uint32_t check) {
	return check <= length();
}