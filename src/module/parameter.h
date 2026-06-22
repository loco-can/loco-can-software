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
  }

#endif

// =======================================

#endif