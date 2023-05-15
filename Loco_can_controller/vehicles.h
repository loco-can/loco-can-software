/*
 * LOCO-CAN Controller vehicle methods
 * 
 * @author: Thomas H Winkler
 * @copyright: 2023
 * @lizence: GG0
 */

/*
 * The class creates an list of module uuids. Every entry has a timeout for automatic
 * deletion, so the list represents the current uuids in a timeout period
 */


#ifndef VEHICLES_H
#define VEHICLES_H

#include <Arduino.h>
#include "measure_calculate.h"


#define VEHICLES_MAX_COUNT 16						// max count of registerable vehivles
#define VEHICLES_LIVETIME 1000						// timeout to remove vehicles from list


// operation types for status bit check
#define VEHICLE_STATUS_OR 0
#define VEHICLE_STATUS_AND 1
#define VEHICLE_STATUS_XOR 2


struct VEHICLE {
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
		VEHICLE get_vehicle(uint8_t id);			// get vehicle by id
		bool get_status(uint8_t bit, uint8_t op);


	private:

		void _purge(void);			// remove outtimed entries

		VEHICLE _vehicles[VEHICLES_MAX_COUNT];		// list of vehicles
		uint8_t _count;								// count of registered vehicles
		
		// status bit 
		uint8_t _or;
		uint8_t _and;
		uint8_t _xor;

		uint8_t _i;
};


#endif