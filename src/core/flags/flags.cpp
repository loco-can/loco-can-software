/*
 * FLAGS class
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */


#include "flags.h"


FLAGS::FLAGS(void) {
}


void FLAGS::begin(void) {
	_flags = 0;
}



// ======================================
// set and get all 8 flags

// set all flags
void FLAGS::set(uint8_t data) {
	_flags = data;
}


// return status byte
uint8_t FLAGS::get(void) {
	return _flags;
}


// ======================================
// set and get single flag


// set bit
void FLAGS::set(uint8_t bit, bool value) {

	// restrict to 8 bits
	if (value < 8) {
		_flags ^= (-value ^ _flags) & (1UL << bit);
	}
}

bool FLAGS::get(uint8_t bit) {
	return (_flags >> bit) & 1;
}