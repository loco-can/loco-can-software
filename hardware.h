/*
 * LOCO-DRIVE hardware selection file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2026
 * @lizence: GG0
 *
 * Version 3.0
 */
#pragma once

#ifndef HARDWARE_H
#define HARDWARE_H


/* ========================================================================
 * HARDWARE SELECTION
 *
 * Select the harware module and its version to compile the sofware.
 * Uncomment the corresponding line of the module and the used harware version. 
 */

 /* ===== MODULE TYPES ===== */
 #define MODULE_TYPE_CONFIGURATOR 0x0008
 #define MODULE_TYPE_CONTROLLER 0x0001
 #define MODULE_TYPE_ELECTRIC 0x0002
 #define MODULE_TYPE_SWITCH 0x0003
 #define MODULE_TYPE_SENSOR 0x0004
 #define MODULE_TYPE_SERVO 0x0005
 #define MODULE_TYPE_LED 0x0006
 #define MODULE_TYPE_DRIVE 0x0007
// #define MODULE_TYPE_UNIVERSAL 0x0000
 /* ===================================================================== */


/* ========================================================================
 * UNIVERSAL MODULE
 *
 * The universal module is a small PCB without dedicated hardware. Beside
 * the power supply and the CAN communication only general IOs are accesed
 * by pin headers.
 * The universal module can be used for all modules. 
 */
// #define MODULE UNIVERSAL_MODULE

/* ===== MODULE VERSIONS ===== */
// #define UNIVERSAL_MODULE_VERSION V_1_0
// #define UNIVERSAL_MODULE_VERSION V_2_0
// #define UNIVERSAL_MODULE_VERSION V_2_1
// #define UNIVERSAL_MODULE_VERSION V_3_0
/* ===================================================================== */


/* ========================================================================
 * CONTROLLER MODULE
 *
 * The controller module is based on a universal hardware combined with
 * an adapter PCB with sockets for the potentiometer and switches.
 * It is used to control locomotives with an electic or combution motor.
 */
#define MODULE CONTROLLER_MODULE
#define MODULE_TYPE MODULE_TYPE_CONTROLLER

/* ===== MODULE VERSIONS ===== */
// #define HARDWARE_VERSION V_2_0
#define HARDWARE_VERSION V_2_1
/* ===================================================================== */

/* ========================================================================
 * SWITCH MODULE
 *
 * The switch module offers six 5 Ampere outputs. The outputs can be mapped
 * to all switching operations, that are defined in the CAN protocol.
 * The default mapping is for switching lights. 
 */
// #define MODULE SWITCH_MODULE
// #define MODULE_TYPE MODULE_TYPE_SWITCH

/* ===== MODULE VERSIONS ===== */
// #define HARDWARE_VERSION V_2_0
// #define HARDWARE_VERSION V_2_1
/* ===================================================================== */


/* ========================================================================
 * SENSOR MODULE
 *
 * The sensor module offers different sensors for electric values from
 * voltage and electric current to pulse measuring for speed or rpm values.
 */
// #define MODULE SENSOR_MODULE
// #define MODULE_TYPE MODULE_TYPE_SENSOR

/* ===== MODULE VERSIONS ===== */
// #define HARDWARE_VERSION V_2_0
// #define HARDWARE_VERSION V_2_1
/* ===================================================================== */


/* ========================================================================
 * LED MODULE
 *
 * This module is not yet implemented.
 *
 * The module can drive LEDs with different brightnes and color.
 */
// #define MODULE LED_MODULE
// #define MODULE_TYPE MODULE_TYPE_LED

/* ===== MODULE VERSIONS ===== */
// #define HARDWARE_VERSION V_2_0
/* ===================================================================== */


/* ========================================================================
 * DRIVE MODULE
 *
 * The drive module is a 4-Q controller for electric locomotives with an
 * integrated CAN bus interface. In addition potiometers and seitches as
 * well as gauges can directly be connected, if the driver sits in the
 * locomotive and teh module is also installed inside the vehicle. In this
 * case the loco is controlled directly, but another loco can be controlled
 * via the CAN bus.
 */
// #define MODULE DRIVE_MODULE
// #define MODULE_TYPE MODULE_TYPE_DRIVE

/* ===== MODULE VERSIONS ===== */
// #define HARDWARE_VERSION V_2_0
/* ===================================================================== */


/* ========================================================================
 * CONFIGURATOR MODULE
 *
 * Tool module for bus discovery and module setup. Sends empty pings on
 * CAN_ID_PING and listens for version replies from other modules.
 */
// #define MODULE CONFIGURATOR_MODULE
// #define MODULE_TYPE MODULE_TYPE_CONFIGURATOR

/* ===== MODULE VERSIONS ===== */
// #define HARDWARE_VERSION V_1_0
// #define HARDWARE_VERSION V_3_0
/* ===================================================================== */


/* ====================================================================== */
// INCLUDE MODULE CLASS (defines ANALOGSWITCH_MAX_POS and module settings)
/* ====================================================================== */
#include MODULE


/* include the module parameters */
#include "src/module/parameter.h"


#endif