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

	/* SSD1351 128x128 RGB OLED on VSPI */
	#define PROGRAMMER_OLED_SCK 18
	#define PROGRAMMER_OLED_MOSI 23
	#define PROGRAMMER_OLED_CS 2
	#define PROGRAMMER_OLED_DC 21
	#define PROGRAMMER_OLED_RST 22
	#define PROGRAMMER_OLED_WIDTH 128
	#define PROGRAMMER_OLED_HEIGHT 128

	/* KY-040 rotary encoder with pushbutton */
	#define PROGRAMMER_ENC_A 25
	#define PROGRAMMER_ENC_B 26
	#define PROGRAMMER_ENC_BTN 27

	#define PROGRAMMER_WIFI_SSID "Loco-CAN-Programmer"
	#define PROGRAMMER_WIFI_PASS "lococanprg"
	#define PROGRAMMER_HTTP_PORT 80

	#define PROGRAMMER_GROUP_MAX 48
	#define PROGRAMMER_OLED_ROWS 7

	#define PROGRAMMER_IO1 5
	#define PROGRAMMER_IO2 19
	#define PROGRAMMER_IO3 33
	#define PROGRAMMER_IO4 15
	#define PROGRAMMER_IO14 14
	#define PROGRAMMER_IO15 13
	
#elif defined(PROGRAMMER_MODULE_VERSION)
	#error "Set PROGRAMMER_MODULE_VERSION in hardware.h (V_2_0_0)"
#endif

#if defined(PROGRAMMER_MODULE_VERSION)
#include "main.h"
#endif

#endif
