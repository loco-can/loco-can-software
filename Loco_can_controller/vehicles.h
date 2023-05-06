/*
 * LOCO-CAN Controller vehicle methods
 * 
 * @author: Thomas H Winkler
 * @copyright: 2023
 * @lizence: GG0
 */

/*
 * The class creates an list of module uuids. Every entry has a timeout for automatic
 * deletiion, so the list represents the current uuids in a timeout period
 */


#ifndef VEHICLES_H
#define VEHICLES_H

#include <Arduino.h>
#include "measure_calculate.h"


#define VEHICLES_MAX_COUNT 16						// max count of registerable vehivles
#define VEHICLES_LIVETIME 1000						// timeout to remove vehicles from list


// operation types for status bit check
#define VEHICEL_STATUS_OR 0
#define VEHICEL_STATUS_AND 1
#define VEHICEL_STATUS_XOR 2


struct VEHICEL {
	uint16_t uuid;
	uint8_t time;
	uint8_t status;
};


class VEHICLES {

	public:
		void begin(void);

		void reset(void);							// clear list
		uint8_t add(uint16_t uuid);					// add vehicle to list
		uint8_t add(uint16_t uuid, uint8_t status);	// add vehicle with status to list
		uint8_t exists(uint16_t uuid);				// check if vehicle already exists
		uint8_t count(void);						// get count of registered vehicles
		bool get_status(uint8_t bit, uint8_t op);

	private:

		void _purge(void);			// remove outtimed entries

		VEHICEL _vehicle[VEHICLES_MAX_COUNT];		// list of vehicles
		uint8_t _count;								// count of registered vehicles
		
		// status bit 
		uint8_t _or;
		uint8_t _and;
		uint8_t _xor;

		MEASURE_CALCULATE _voltage;
		MEASURE_CALCULATE _current;
		MEASURE_CALCULATE _speed;

		uint8_t _i;
};


#endif