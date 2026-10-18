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

/* 8-bit type: bit7 wireless, bits6-4 group, bits3-0 type (doc/SETTINGS.md). */
#define MODULE_TYPE TYPE_PROGRAMMER


#if defined(PROGRAMMER_MODULE_VERSION) && LOCO_VERSION_IS(PROGRAMMER_MODULE_VERSION, 2, 0)

	// ======================================
	// ESP32-WROOM-32
	// ======================================
	#define MODULE_ARCH_ESP32

	#define PLATFORM_ANALOG_RESOLUTION 4096
	#define ANALOGSWITCH_MAX_POS 8

	/*
	 * GPIO 6-11 are the WROOM-32 flash bus.
	 * GPIO 34-39 have no internal pull-up; the TWAI driver needs one on RX.
	 * GPIO 5 is VSPI default SS (output).
	 * Transceiver RXD → CAN_RX (GPIO 4), TXD → CAN_TX (GPIO 17).
	 * RX is RMT CH2. TX uses TWAI NO_ACK (GPIO 17 is UART2 by default).
	 */
	#define CAN_RX 4
	#define CAN_TX 17
	#if CAN_TX >= 34 && CAN_TX <= 39
		#error "CAN_TX cannot be GPIO 34-39 (input-only)"
	#endif
	#define CAN_STATUS_LED 16
	#define CAN_MAX_FILTER 16
	#define CAN_BUFFER_SIZE 16

	/* ST7789 240x240 colour OLED on VSPI (SCK 18, MOSI 23) */
	#define PROGRAMMER_OLED_CS 2
	#define PROGRAMMER_OLED_DC 21
	#define PROGRAMMER_OLED_RST 22
	#define PROGRAMMER_OLED_BL 32

	/* KY-040 rotary encoder with pushbutton */
	#define PROGRAMMER_ENC_A 25
	#define PROGRAMMER_ENC_B 26
	#define PROGRAMMER_ENC_BTN 27

	#define PROGRAMMER_WIFI_SSID "Loco-CAN-Programmer"
	#define PROGRAMMER_WIFI_PASS "lococanprg"
	#define PROGRAMMER_HTTP_PORT 80

	#define PROGRAMMER_GROUP_MAX 48
	#define PROGRAMMER_OLED_ROWS 6

#elif defined(PROGRAMMER_MODULE_VERSION) && LOCO_VERSION_IS(PROGRAMMER_MODULE_VERSION, 3, 0)

	// ======================================
	// BASIC SETTINGS
	// ======================================
	#define MODULE_ARCH_ESP32

	#define PLATFORM_ANALOG_RESOLUTION 4096
	#define ANALOGSWITCH_MAX_POS 8

	#define CAN_RX 4
	#define CAN_TX 5
	#define CAN_STATUS_LED 16
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

#elif defined(PROGRAMMER_MODULE_VERSION)
	#error "Set PROGRAMMER_MODULE_VERSION in hardware.h (V_2_0_0 or V_3_0_0)"
#endif

#if defined(PROGRAMMER_MODULE_VERSION)
#include "main.h"
#endif

#endif
