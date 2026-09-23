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
// 0 is the learned point with the lowest voltage
uint8_t ANALOGSWITCH::get(void) {

	uint16_t analogval = get_analog();

	if (_pos_count == 0) {
		return 0;
	}

	uint8_t nearest = 0;
	uint16_t best = 0xFFFF;

	for (uint8_t i = 0; i < _pos_count; i++) {
		uint16_t pos = _positions[i];
		uint16_t dist = (analogval > pos) ? (uint16_t)(analogval - pos) : (uint16_t)(pos - analogval);

		if (dist < best) {
			best = dist;
			nearest = i;
		}
	}

	// _positions is sorted descending, so flip the index
	return (uint8_t)((_pos_count - 1) - nearest);
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
	uint16_t va = *(const uint16_t *)a;
	uint16_t vb = *(const uint16_t *)b;

	if (va < vb) {
		return 1;
	}
	if (va > vb) {
		return -1;
	}
	return 0;
}


// get analog value
uint16_t ANALOGSWITCH::get_analog(void) {

	if (_port) {
		return analogRead(_port);
	}

	return false;
}
