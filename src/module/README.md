# Modules

The modules directory is the place for all modules. Each module can use
Loco-CAN core functions, that are stored in the src/core directory.

The module hardware platform must be defined as MODULE_ARCH_AVR or MODULE_ARCH_ESP32

With the MODULE_VERSION different hardware versions can be distinguished in the
parameter section.

## Parameters

  parameter.h

The parameters are values, that are permanently stored in the EEPROM od the module and hold the settings for the correct function. The parameters can be changed using the configurator or native methods of the module itself.