#pragma once

#ifnded PARAMETER_H
#define PARAMETER_H

#include <inttypes.h>

// =======================================
// common module data
  struct MODULE_TYPE {
    char type; // module type id
	   int software_version;
	   int hardware_version;
  };

// =======================================
// controller module parameters definition
#ifdef CONTROLLER_MODULE_CLASS

  struct {
    struct PARAM_CONTROLLER {
      MODULE_TYPE type;
	     int mains_points[ANALOGSWITCH_MAX_POS]; // settings for mains analogswitch
	     int dir_points[3]; // settings of direction analogswitch
	     char status_mode; // led, oled status type
	     char drive_mode; // drive mode: single, drive/break, speed/power/break
    }

#endif

// =======================================


// =======================================
// electro locomotive paramerer definition

#ifdef ELECTRIC_MODULE_CLASS

  struct PARAM_ELECTRIC {
    MODULE_TYPE type;
    int paired; // uuid of paired controller
    int multi_traction; // 0=single, 1=leading engine, 2=supporting engine
    int tacho_pulses; // tacho pulses per meter
    int battery_type; // battery type (1x12V/24V, 2x12V in serie) 
  }

#endif

// =======================================

#endif