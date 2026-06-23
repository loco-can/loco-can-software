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


ANALOGSWITCH::ANALOGSWITCH(void) {
	_port = false;
}


// start at port number
void ANALOGSWITCH::begin(uint8_t port) {
	_port = port;

	pinMode(_port, INPUT);
}


// get switch position
uint8_t ANALOGSWITCH::get(void) {

	uint8_t i = 0;

	// get analog value
	uint16_t analogval = get_analog();

	// calculate switch position
	// iterate positions
	while (i < _pos_count) {

		// first entry
		if (i == 0) {

			if (analogval < ((_positions[i] + _positions[i + 1]) / 2)) {
				return i;
			}
		}

		// last entry
		else if (i == _pos_count) {

			if (analogval >= ((_positions[i - 1] + _positions[i]) / 2) && analogval < ((_positions[i] + _positions[i + 1]) / 2)) {
				return i;
			}
		}

		// entry between
		else {

			if (analogval >= ((_positions[i - 1] + _positions[i]) / 2) && analogval <= PLATFORM_ANALOG_RESOLUTION) {
				return i;
			}
		}

		i++;
	}

	return i;
}


// learn position
// returns new position count
uint8_t ANALOGSWITCH::learn() {

	// has free value
	if (_pos_count < ANALOGSWITCH_MAX_POS) {
		_positions[_pos_count] = get_analog();
		_pos_count++;
	}

	_sort();

	return _pos_count;
}


uint8_t ANALOGSWITCH::learn(uint16_t value) {

	// has free value
	if (_pos_count < ANALOGSWITCH_MAX_POS) {
		_positions[_pos_count] = value;
		_pos_count++;
	}

	_sort();

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

	_sort();

	return _pos_count;
}


// sort positions ascending
void ANALOGSWITCH::_sort(void) {

	qsort(_positions, _pos_count, sizeof(_positions[0]), _compare);

}

int ANALOGSWITCH::_compare(const void *a, const void *b) {
    return (*(int *)b - *(int *)a);
}


// get analog value
uint16_t ANALOGSWITCH::get_analog(void) {

	if (_port) {
		return analogRead(_port);
	}

	return false;
}
