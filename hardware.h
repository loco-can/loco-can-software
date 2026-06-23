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

/* ===== MODULE VERSIONS ===== */
// #define CONTROLLER_MODULE_VERSION V_2_0
#define CONTROLLER_MODULE_VERSION V_2_1
/* ===================================================================== */


/* ========================================================================
 * MOTOR MODULE
 *
 * The motor module connects the CAN control with a electro motor driver.
 * It has no power amplifier but only control lines to operate different
 * motor drivers.
 */
// #define MODULE ELECTRIC_MODULE

/* ===== MODULE VERSIONS ===== */
// #define ELECTRIC_MODULE_VERSION V_1_0
// #define ELECTRIC_MODULE_VERSION V_2_0
/* ===================================================================== */


/* ========================================================================
 * SWITCH MODULE
 *
 * The switch module offers six 5 Ampere outputs. The outputs can be mapped
 * to all switching operations, that are defined in the CAN protocol.
 * The default mapping is for switching lights. 
 */
// #define MODULE SWITCH_MODULE

/* ===== MODULE VERSIONS ===== */
// #define SWITCH_MODULE_VERSION V_2_0
// #define SWITCH_MODULE_VERSION V_2_1
/* ===================================================================== */


/* ========================================================================
 * SENSOR MODULE
 *
 * The sensor module offers different sensors for electric values from
 * voltage and electric current to pulse measuring for speed or rpm values.
 */
// #define MODULE SENSOR_MODULE

/* ===== MODULE VERSIONS ===== */
// #define SENSOR_MODULE_VERSION V_2_0
// #define SENSOR_MODULE_VERSION V_2_1
/* ===================================================================== */


/* ========================================================================
 * LED MODULE
 *
 * This module is not yet implemented.
 *
 * The module can drive LEDs with different brightnes and color.
 */
// #define MODULE LED_MODULE

/* ===== MODULE VERSIONS ===== */
// #define LED_MODULE_VERSION V_2_0
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

/* ===== MODULE VERSIONS ===== */
// #define DRIVE_MODULE_VERSION V_2_0
/* ===================================================================== */


/* ========================================================================
 * CONFIGURATOR MODULE
 *
 * Tool module for bus discovery and module setup. Sends empty pings on
 * CAN_ID_PING and listens for version replies from other modules.
 */
// #define MODULE CONFIGURATOR_MODULE

/* ===== MODULE VERSIONS ===== */
// #define CONFIGURATOR_MODULE_VERSION V_1_0
// #define CONFIGURATOR_MODULE_VERSION V_3_0
/* ===================================================================== */


/* ====================================================================== */
// ACTIVE MODULE VERSION AND TYPE (for CAN ping / setup)
/* ====================================================================== */
#if defined(CONTROLLER_MODULE_VERSION)
	#define HARDWARE_VERSION CONTROLLER_MODULE_VERSION
	#define MODULE_TYPE_ID 0x0001
#elif defined(ELECTRIC_MODULE_VERSION)
	#define HARDWARE_VERSION ELECTRIC_MODULE_VERSION
	#define MODULE_TYPE_ID 0x0002
#elif defined(SWITCH_MODULE_VERSION)
	#define HARDWARE_VERSION SWITCH_MODULE_VERSION
	#define MODULE_TYPE_ID 0x0003
#elif defined(SENSOR_MODULE_VERSION)
	#define HARDWARE_VERSION SENSOR_MODULE_VERSION
	#define MODULE_TYPE_ID 0x0004
#elif defined(SERVO_MODULE_VERSION)
	#define HARDWARE_VERSION SERVO_MODULE_VERSION
	#define MODULE_TYPE_ID 0x0005
#elif defined(LED_MODULE_VERSION)
	#define HARDWARE_VERSION LED_MODULE_VERSION
	#define MODULE_TYPE_ID 0x0006
#elif defined(DRIVE_MODULE_VERSION)
	#define HARDWARE_VERSION DRIVE_MODULE_VERSION
	#define MODULE_TYPE_ID 0x0007
#elif defined(CONFIGURATOR_MODULE_VERSION)
	#define HARDWARE_VERSION CONFIGURATOR_MODULE_VERSION
	#define MODULE_TYPE_ID 0x0008
#elif defined(UNIVERSAL_MODULE_VERSION)
	#define HARDWARE_VERSION UNIVERSAL_MODULE_VERSION
	#define MODULE_TYPE_ID 0x0000
#else
	#define HARDWARE_VERSION 0
	#define MODULE_TYPE_ID 0x0000
#endif


/* include the module parameters */
#include "src/module/parameter.h"


/* ====================================================================== */
// INCLUDE MODULE CLASS
/* ====================================================================== */
#include MODULE

#endif