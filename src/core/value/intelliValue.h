/*
 * INTELLIVALUE CLASS
 *
 * package format
 *    package[4]
 *      byte 0: bit 3-7 = index
 *              bit 0-2 = percentage HIGH (bits 8-10)
 *      byte 1: bit 0-7 = percentage LOW (bits 0-7)
 *      byte 2: reference HIGH
 *      byte 3: reference LOW
 */

/*
 * usage:
 *
 * The value is represented by an reference value and a precentage. The 16 bit
 * reference is set in the begin method. A precission is calculated to exploit the
 * full 16 bit range and is accessable by the precission() method. A reference of 12 (i.e. Volt)
 * is automatically set to a precission of 3, which means, that a absolute value
 * is returned with three decimals. A 12 Volt value is returne as an integer of
 * 12000, 8.836 Volts are returne as 8836 (precission 3).
 *
 * The percentage value relates to the reference value. A absolute value is converted
 * to a percentage in 1/100 percent. 100% equals percentage = 10000. The percentage is a 16 bit
 * value, hence maximal values of 205% of the reference value can be handled.
 */
#pragma once


struct MEASURE_VALUE {
	uint16_t reference;		// 16 bit reference value
	uint8_t precission;		// precission: number of decimal places in the integer percentage value
	uint16_t percentage;	// 11 bit used: 1/10 percent 0-205% = 0-2048
	uint8_t index;			// index for similar values 0-255 (0=default)
};



class INTELLIVALUE {

	public:
		// begin = true: index available, value set

	    // init and set reference value (precission = 0)
		bool begin(uint16_t reference);

		// init with additional index
   		bool begin(uint16_t reference, uint8_t index);

		void set(uint16_t value);           // set absolute value
		uint16_t get(void);                 // get absolute value

		void set_package(char* package);    // set from 4 byte package
		void get_package(char*);            // get data as 4 byte package

		uint16_t reference(void);           // get reference (16 bit)
		void reference(uint16_t);           // aet reference (16 bit)

		uint16_t precission(void);           // get precission (8 bit)
		void precission(uint16_t);           // aet precission (8 bit)

		uint16_t percentage(void);          // get percent of reference value (11 bit)
		void percentage(uint16_t);          // set percent of reference value (11 bit)

		uint8_t index(void);                // get index
		void index(uint8_t);                // set index

	private:
		MEASURE_VALUE _value;
};
