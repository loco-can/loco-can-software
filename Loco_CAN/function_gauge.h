#ifndef FUNCTION_GAUGE_H
#define FUNCTION_GAUGE_H

/*
 * Loco-CAN gauge function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 * The gauge function can handle different types of gauges to display measure
 * values, like voltage, current, speed, rotation, temperature, i.e.
 * Gauges can receive analog values or model servo signals.
 *
 * future extension: It also supports different kinds of OLED or LCD displays.
 */


class FUNCTION_GAUGE {

public:
	void begin(void);
};


#endif