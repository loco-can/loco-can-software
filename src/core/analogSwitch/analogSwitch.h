/*
 * Analog Switch Library Header
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */

/*
 * With the library an analog input can be used
 * for a multi position switch 
 */
#pragma once

#ifndef ANALOGSWITCH_H
#define ANALOGSWITCH_H


#include <Arduino.h>
#include "../../config.h"


class ANALOGSWITCH {

	public:

		ANALOGSWITCH(void);

		void begin(uint8_t port); // set the port

		uint8_t learn(void); // learn the position from analog input
		uint8_t learn(uint16_t); // store a position from value

		uint8_t remove(uint8_t); // remove position with index idx

		uint8_t get(void); // get the switch position
		uint16_t get_analog(void); // get the analog value

	private:
		uint8_t _port; // port number
		uint16_t _positions[ANALOGSWITCH_MAX_POS]; // up to 16 positions
		uint8_t _pos_count; // count of stored positions

		void _sort(void); // sort the positions
		static int _compare(const void*, const void*); // compare method for qsort
};

#endif