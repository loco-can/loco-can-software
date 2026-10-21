/*
 * Analog Switch Library
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020-2025
 * @lizence: GG0
 */

/*
 * With the library an analog input can be used
 * for a multi position switch 
 */


#include "../../config.h"
#include "analogSwitch.h"
#include "analog_switch_logic.h"


ANALOGSWITCH::ANALOGSWITCH(void) {
	_port = 0;
	_pos_count = 0;
}


// start at port number
void ANALOGSWITCH::begin(uint8_t port) {
	_port = port;
	_pos_count = 0;

	pinMode(_port, INPUT);
}


// get switch position
// 0 is the first programmed point (EEPROM / learn order)
uint8_t ANALOGSWITCH::get(void) {

	uint16_t analogval = get_analog();

	return analog_switch_nearest(_positions, _pos_count, analogval);
}


// learn position
// returns new position count
uint8_t ANALOGSWITCH::learn() {

	// has free value
	if (_pos_count < ANALOGSWITCH_MAX_POS) {
		_positions[_pos_count] = get_analog();
		_pos_count++;
	}

	return _pos_count;
}


uint8_t ANALOGSWITCH::learn(uint16_t value) {

	// has free value
	if (_pos_count < ANALOGSWITCH_MAX_POS) {
		_positions[_pos_count] = value;
		_pos_count++;
	}

	return _pos_count;
}


// remove position with index n
uint8_t ANALOGSWITCH::remove(uint8_t idx) {

	// index is valid and count not null
	if (idx < _pos_count && _pos_count > 0) {

		uint8_t i = idx;

		while (i<_pos_count) {
			_positions[i] = _positions[i+1];
			i++;
		}

		_pos_count--;
	}

	return _pos_count;
}


void ANALOGSWITCH::set_positions(const uint16_t *points, uint8_t count) {

	if (count > ANALOGSWITCH_MAX_POS) {
		count = ANALOGSWITCH_MAX_POS;
	}

	_pos_count = 0;

	if (points == 0) {
		return;
	}

	for (uint8_t i = 0; i < count; i++) {
		_positions[i] = points[i];
		_pos_count++;
	}

}


uint8_t ANALOGSWITCH::count(void) {
	return _pos_count;
}


// logical index 0 is the first programmed point
uint16_t ANALOGSWITCH::position(uint8_t logical_index) {

	if (_pos_count == 0 || logical_index >= _pos_count) {
		return 0;
	}

	return _positions[logical_index];
}


// get analog value
uint16_t ANALOGSWITCH::get_analog(void) {

	if (_port) {
		return analogRead(_port);
	}

	return false;
}
