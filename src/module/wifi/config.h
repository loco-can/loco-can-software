/*
 * Loco-CAN WIFI module
 *
 * @author: Thomas H Winkler
 * @copyright: 2026
 * @lizence: GG0
 *
 * ESP32 wireless CAN bridge. Two boards pair with ESP-NOW and copy
 * CAN frames between isolated bus segments when no physical cable
 * joins them.
 */

#pragma once

#ifndef MODULE_WIFI_CONFIG_H
#define MODULE_WIFI_CONFIG_H


#if WIFI_MODULE_VERSION == V_3_0

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_ESP32

	#define PLATFORM_ANALOG_RESOLUTION 4096
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_RX 17
	#define CAN_TX 18
	#define CAN_STATUS_LED 46
	#define CAN_MAX_FILTER 16
	#define CAN_BUFFER_SIZE 16

	// ======================================
	// INCLUDED FUNCTIONS
	// ======================================

	/* highest bit set = wireless capable (see doc/SETTINGS.md) */
	#define WIFI_MODULE_TYPE 0x81

	#define WIFI_NOW_CHANNEL 1
	#define WIFI_NOW_HELLO_MS 500
	#define WIFI_NOW_PEER_TIMEOUT_MS 2000
	#define WIFI_NOW_CABLE_TIMEOUT_MS 750
	#define WIFI_NOW_QUEUE 16
	#define WIFI_RECENT_MAX 24

#else
	#error "Set WIFI_MODULE_VERSION in hardware.h (V_3_0)"
#endif

#include "main.h"

#endif
