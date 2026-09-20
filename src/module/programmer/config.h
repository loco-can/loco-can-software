/*
 * Loco-CAN programmer module
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2026
 * @lizence: GG0
 *
 * ESP32 programmer with colour OLED, rotary encoder and WiFi webservice.
 * Copy this directory to loco-can-software/src/module/programmer/
 * then select PROGRAMMER_MODULE in hardware.h.
 */

#pragma once

#ifndef MODULE_PROGRAMMER_CONFIG_H
#define MODULE_PROGRAMMER_CONFIG_H


#if PROGRAMMER_MODULE_VERSION == V_3_0

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

	/* ST7789 240x240 colour OLED (SPI) */
	#define PROGRAMMER_OLED_CS 10
	#define PROGRAMMER_OLED_DC 9
	#define PROGRAMMER_OLED_RST 8
	#define PROGRAMMER_OLED_BL 7

	/* KY-040 rotary encoder with pushbutton */
	#define PROGRAMMER_ENC_A 5
	#define PROGRAMMER_ENC_B 6
	#define PROGRAMMER_ENC_BTN 4

	/* WiFi access point for the webservice (WPA2 password must be ≥ 8 chars) */
	#define PROGRAMMER_WIFI_SSID "LOCO-CAN-PRG"
	#define PROGRAMMER_WIFI_PASS "lococanprg"
	#define PROGRAMMER_HTTP_PORT 80

	#define PROGRAMMER_GROUP_MAX 48
	#define PROGRAMMER_OLED_ROWS 6

#else
	#error "Set PROGRAMMER_MODULE_VERSION in hardware.h (V_3_0)"
#endif

#include "main.h"

#endif
