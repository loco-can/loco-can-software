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


 #include "vehicles.h"


// start vehicle list
void VEHICLES::begin(void) {
	reset();
}


// add vehicle and remove outtimed entries
// return number of new entry
uint8_t VEHICLES::add(uint16_t uuid) {
	add(uuid, 0);
}


// add with status
// purge timeout entries
uint8_t VEHICLES::add(uint16_t uuid, uint8_t status) {

	// has free places
	if (count() < VEHICLES_MAX_COUNT) {


		// vehicle already exists => retrigger time
		if ((_i = exists(uuid)) > 0) {
			_vehicles[_i].time = millis();
		}

		// new vehicle
		else {

			// find free place and add new vehicle
			for (_i = 0;_i < VEHICLES_MAX_COUNT; _i++) {

				if (_vehicles[_i].uuid == 0) {
					_vehicles[_i].uuid = uuid;
					_vehicles[_i].time = millis();

					break;
				}
			}
		}
	}

	// purge and recount
	_purge();

	return _i;
}


// check if vehicle is present
// return number of entry (1...x)
// return 0 if not exists
uint8_t VEHICLES::exists(uint16_t uuid) {

	for (_i = 0;_i < VEHICLES_MAX_COUNT; _i++) {

		if (_vehicles[_i].uuid == uuid) {
			break;
		}
	}

	return _i + 1;
}


// clear list
void VEHICLES::reset(void) {

	 for (_i = 0; _i < VEHICLES_MAX_COUNT; _i++) {
	 	_vehicles[_i].uuid = 0;
	 	_vehicles[_i].time = 0;
	 }

	 _count = 0;
}


// purge list by timeout
// update count
void VEHICLES::_purge(void) {

	_count = 0;
	_or = 0;
	_and = 0;
	_xor = 0;

	for (_i = 0; _i < VEHICLES_MAX_COUNT; _i++) {

	 	// vehicle exists
	 	if (_vehicles[_i].uuid != 0) {

	 		// timed out => purge
	 		if((_vehicles[_i].time + VEHICLES_LIVETIME) >= millis()) {
		 		_vehicles[_i].uuid = 0;
		 		_vehicles[_i].time = 0;
		 	}

		 	else{	 		
			 	// count
				_count++;

				// calculate bit operators
				_or |= _vehicles[_i].status;
				_and &= _vehicles[_i].status;
				_xor ^= _vehicles[_i].status;
		 	}
		}
	}
}


// update count
// return new count
uint8_t VEHICLES::count(void) {
	return _count;
}


// get vehicle by id
VEHICLE VEHICLES::get_vehicle(uint8_t id) {

	if (id < VEHICLES_MAX_COUNT) {
		return _vehicles[id];
	}
}


// check summonded status of all registered vehicles bits by function
// bit = bit of status (see status bit list in LocoCAN library)
bool VEHICLES::get_status(uint8_t bit, uint8_t op) {

	bool ret = false;

	// valid bit
	if (bit < 8) {

		switch(op) {

			case VEHICLE_STATUS_OR:
				ret = (bool)(_or >> bit);
				break;

			case VEHICLE_STATUS_AND:
				ret = (bool)(_and >> bit);
				break;

			case VEHICLE_STATUS_XOR:
				ret = (bool)(_xor >> bit);
				break;
		}
	}

	return ret;
}