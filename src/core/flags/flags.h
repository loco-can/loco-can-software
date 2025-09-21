/*
 * LOCO-CAN LOCO status class header
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020-2025
 * @lizence: GG0
 */

#pragma once

#include <Arduino.h>


class FLAGS {

	public:
		FLAGS(void);
		void begin(void);

		void set(uint8_t data); // set complete byte
		uint8_t get(void); // get status byte

		void set(uint8_t bit, bool value); // set bit of _flags to value
		bool get(uint8_t bit);

	private:
		uint8_t _flags; // stati

};
