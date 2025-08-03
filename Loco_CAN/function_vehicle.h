#ifndef FUNCTION_VEHICLE_H
#define FUNCTION_VEHICLE_H

/*
 * Loco-CAN vehicle function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 * The vehicle function is the interface between a vehicles propulsion system
 * and the controller. It manages the interaction between the vehicle
 * and the controller. This function does not define the type of the engine
 * system of the locomotive.
 */


class FUNCTION_VEHICLE {

public:
	void begin(void);
};


#endif