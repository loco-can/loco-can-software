/*
 * Loco-CAN programmer function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2026
 * @lizence: GG0
 *
 * FUNCTIONALITY
 *   Analyse the CAN bus: fold frames with the same ID and UUID, translate
 *   the group identifier to a plain-text name and show payload fields as
 *   integers or boolean switches.
 *
 *   Setup: send a global 0x7FF ping, list answering modules and write
 *   EEPROM parameters back with 0x7nn frames.
 *
 *   Local UI is a colour OLED plus rotary encoder. The same functions are
 *   exposed by the ESP32 WiFi access point webservice.
 */

#pragma once

#ifndef MODULE_PROGRAMMER_H
#define MODULE_PROGRAMMER_H

#include "../../config.h"
#include "../../core/can/can_com.h"
#include "../../can_protocol.h"
#include "../../core/button/intelliButton.h"
#include "../../core/timeout/intellitimeout.h"

extern CAN_COM can;

#define PROGRAMMER_MODE_ANALYSE 0
#define PROGRAMMER_MODE_SETUP 1

struct PROGRAMMER_GROUP {
	uint16_t id;
	uint16_t uuid;
	uint16_t count;
	uint32_t last_ms;
	CAN_MESSAGE last;
	bool used;
};

struct PROGRAMMER_NODE {
	uint16_t uuid;
	uint8_t type;
	uint8_t version_maj;
	uint8_t version_min;
	char name[16];
	uint8_t max_settings;
	bool used;
};

class MODULE_PROGRAMMER {

	public:
		void begin(void);
		void update(CAN_MESSAGE message);

	private:
		uint8_t _mode;
		int16_t _cursor;
		PROGRAMMER_GROUP _groups[PROGRAMMER_GROUP_MAX];
		PROGRAMMER_NODE _nodes[16];
		INTELLIBUTTON _encoder_btn;
		INTELLITIMEOUT _oled_time;
		INTELLITIMEOUT _ping_time;

		void _ingest(CAN_MESSAGE message);
		void _poll_encoder(void);
		void _draw_oled(void);
		void _start_ap(void);
		void _ping(void);
		int8_t _find_group(uint16_t id, uint16_t uuid);
};

#endif
