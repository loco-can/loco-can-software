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


#define VEHICLES_MAX_COUNT 16						// max count of registerable vehivles
#define VEHICLES_LIVETIME 1000						// timeout to remove vehicles from list


struct VEHICEL {
	uint16_t uuid;
	uint8_t time;
};


class VEHICLES {

	public:
		void begin(void);

		void reset(void);							// clear list
		uint8_t add(uint16_t uuid);					// add vehicle to list
		uint8_t exists(uint16_t uuid);				// check if vehicle already exists
		uint8_t count(void);						// get count of registered vehicles

	private:

		void _purge(void);			// remove outtimed entries

		VEHICEL _vehicle[VEHICLES_MAX_COUNT];		// list of vehicles
		uint8_t _count;								// count of registered vehicles

		uint8_t _i;
};


#endif