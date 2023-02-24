#ifndef STATUS_H
#define STATUS_H

#include <Arduino.h>


class STATUS {
	public:
		void begin(uint8_t); // init status
		void set(uint8_t); // set status to
		uint8_t get(void); // get status
		bool is(uint8_t); // check if has status
		uint8_t count(void); // return max count of stati
		uint8_t next(void); // step to next status
		uint8_t previous(void); // step to previous status

	private:
		uint8_t _status;
		uint8_t _max;
};


#endif