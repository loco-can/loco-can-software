# Modules

The modules directory is the place for all modules. Each module can use
Loco-CAN core functions, that are stored in the src/core directory.

The module hardware platform must be defined as MODULE_ARCH_AVR or MODULE_ARCH_ESP32

With the MODULE_VERSION different hardware versions can be distinguished in the
parameter section.

## Parameters

  parameter.h

Parameters are values that are permanently stored in the EEPROM od the module. They can be changed by the configurator, some also from a controller.

## Controller
- mains analog switch points
- direction analog switch points
- mains pos 2 message (can-id + value)
- mains pos 3 message (can-id + value)
- status display type (led, oled, tft)

## Electric
- paired controller uuid (0 => unpaired)
- direction inverted
- multi traction status (0=single, 1=leading engine, 2=supporting engine)
- tacho pulses per meter
- battery type (1x12V/24V, 2x12V in serie)

The parameters are defined as struct in the parameter.h inside the module subdirectory. The correct parameter definition is enabled via a preprozessor #ifdef clause using the hardware.h selector.
The configurator includes all types to be able to communicate with all module types.

    struct PARAM_CONTROLLER {
      char type; // module type id
      int software_version;
      int hardware_version;
      int mains_points[ANALOGSWITCH_MAX_POS]; // settings for mains analogswitch
      int dir_points[3]; // settings of direction analogswitch
      char status_mode; // led, oled status type
      char drive_mode; // drive mode: single, drive/break, speed/power/break
      }