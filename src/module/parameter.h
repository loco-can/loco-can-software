/*
 * LOCO-DRIVE parameter definition file
 *
 * @author: Thomas H Winkler
 * @copyright: 2026
 * @lizence: GG0
 *
 * Version 1.0: initial version
 *
 * Included from hardware.h after the active module config.h.
 * Define PARAMETER_IMPLEMENTATION in one .cpp to allocate module_parameters.
 */
#pragma once

#ifndef PARAMETER_H
#define PARAMETER_H


#include <Arduino.h>
#include <string.h>

#ifndef ANALOGSWITCH_MAX_POS
	#define ANALOGSWITCH_MAX_POS 8
#endif


#ifdef PARAMETER_IMPLEMENTATION
	#define MODULE_PARAMETERS_DECLARE
#else
	#define MODULE_PARAMETERS_DECLARE extern
#endif


#define MODULE_PARAMETERS_INIT_META(params)          \
	do {                                             \
		(params).type = (char)(MODULE_TYPE & 0xFF); \
		(params).software_version = SOFTWARE_VERSION_NUM; \
		(params).hardware_version = HARDWARE_VERSION; \
	} while (0)


/* ======================================================================== */
// CONTROLLER MODULE PARAMETERS
/* ======================================================================== */
#if MODULE_TYPE == MODULE_TYPE_CONTROLLER

	struct PARAM_CONTROLLER {
		char type;
		int software_version;
		int hardware_version;
		int mains_points[ANALOGSWITCH_MAX_POS];
        char mains_count;
		int dir_points[ANALOGSWITCH_MAX_POS];
		char dir_count;
		char status_mode;
		char drive_mode;
		int paired_uuid;
	};

	MODULE_PARAMETERS_DECLARE PARAM_CONTROLLER module_parameters;
	#define MODULE_PARAMETERS_SIZE sizeof(PARAM_CONTROLLER)

    /**
     * Set the default parameters for the controller module.
     */
	static inline void module_parameters_set_defaults(void) {

		MODULE_PARAMETERS_INIT_META(module_parameters);
		module_parameters.status_mode = 0;
		module_parameters.drive_mode = 0;
		module_parameters.paired_uuid = 0;

        // ================================
        // clear analogswitch points
		memset(module_parameters.mains_points, 0, sizeof(module_parameters.mains_points));
		memset(module_parameters.dir_points, 0, sizeof(module_parameters.dir_points));

        // set default mains points
        module_parameters.mains_points[0] = 0;
        module_parameters.mains_points[1] = PLATFORM_ANALOG_RESOLUTION / 2;
        module_parameters.mains_points[2] = PLATFORM_ANALOG_RESOLUTION; 
        
        module_parameters.mains_count = 3;

        // set default dir points
        module_parameters.dir_points[0] = 0;
        module_parameters.dir_points[1] = PLATFORM_ANALOG_RESOLUTION / 2;
        module_parameters.dir_points[2] = PLATFORM_ANALOG_RESOLUTION;
        
        module_parameters.dir_count = 3;
    }


/* ======================================================================== */
// ELECTRIC MODULE PARAMETERS
/* ======================================================================== */
#elif MODULE_TYPE == MODULE_TYPE_ELECTRIC

	struct PARAM_ELECTRIC {
		char type;
		int software_version;
		int hardware_version;
		bool reverse;
		int paired_uuid;
	};

	MODULE_PARAMETERS_DECLARE PARAM_ELECTRIC module_parameters;
	#define MODULE_PARAMETERS_SIZE sizeof(PARAM_ELECTRIC)

	static inline void module_parameters_set_defaults(void) {

		MODULE_PARAMETERS_INIT_META(module_parameters);
		module_parameters.reverse = false;
		module_parameters.paired_uuid = 0;
	}


/* ======================================================================== */
// SENSOR MODULE PARAMETERS
/* ======================================================================== */
#elif MODULE_TYPE == MODULE_TYPE_SENSOR

	struct PARAM_SENSOR {
		char type;
		int software_version;
		int hardware_version;
	};

	MODULE_PARAMETERS_DECLARE PARAM_SENSOR module_parameters;
	#define MODULE_PARAMETERS_SIZE sizeof(PARAM_SENSOR)

	static inline void module_parameters_set_defaults(void) {

		MODULE_PARAMETERS_INIT_META(module_parameters);
	}


/* ======================================================================== */
// SERVO MODULE PARAMETERS
/* ======================================================================== */
#elif MODULE_TYPE == MODULE_TYPE_SERVO

	struct PARAM_SERVO {
		char type;
		int software_version;
		int hardware_version;
	};

	MODULE_PARAMETERS_DECLARE PARAM_SERVO module_parameters;
	#define MODULE_PARAMETERS_SIZE sizeof(PARAM_SERVO)

	static inline void module_parameters_set_defaults(void) {

		MODULE_PARAMETERS_INIT_META(module_parameters);
	}


/* ======================================================================== */
// SWITCH MODULE PARAMETERS
/* ======================================================================== */
#elif MODULE_TYPE == MODULE_TYPE_SWITCH

	struct PARAM_SWITCH {
		char type;
		int software_version;
		int hardware_version;
	};

	MODULE_PARAMETERS_DECLARE PARAM_SWITCH module_parameters;
	#define MODULE_PARAMETERS_SIZE sizeof(PARAM_SWITCH)

	static inline void module_parameters_set_defaults(void) {

		MODULE_PARAMETERS_INIT_META(module_parameters);
	}


#elif MODULE_TYPE == MODULE_TYPE_CONFIGURATOR

	/* configurator has no local module parameters */

#else
	#error "No module version defined in hardware.h"
#endif


#undef MODULE_PARAMETERS_DECLARE

#endif
