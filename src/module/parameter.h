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
 * ELECTRIC MOTOR MODULE
 *
 * The motor module connects the CAN control with a electro motor driver.
 * It has no power amplifier but only control lines to operate different
 * motor drivers.
 */
// #define MODULE ELECTRIC_MODULE

/* ===== MODULE VERSIONS ===== */
// #define ELECTRIC_MODULE_VERSION V_2_0
// #define ELECTRIC_MODULE_VERSION V_2_1
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
 * SERVO MODULE
 *
 * The servo module can drive up to four analog model servo motors.
 */
// #define MODULE SERVO_MODULE

/* ===== MODULE VERSIONS ===== */
// #define SERVO_MODULE_VERSION V_2_0
// #define SERVO_MODULE_VERSION V_2_1
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

/* ====================================================================== */
// INCLUDE MODULE CLASS
/* ====================================================================== */
#include MODULE

#endif
