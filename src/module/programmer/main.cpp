/*
 * Loco-CAN programmer function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2026
 * @lizence: GG0
 */

#include "main.h"

extern CAN_COM can;


#ifdef MODULE_PROGRAMMER_CONFIG_H

void MODULE_PROGRAMMER::begin(void) {

	#ifdef DEBUG
		Serial.println("*************************");
		Serial.println("start function/programmer");
	#endif

	_mode = PROGRAMMER_MODE_ANALYSE;
	_cursor = 0;

	for (uint8_t i = 0; i < PROGRAMMER_GROUP_MAX; i++) {
		_groups[i].used = false;
	}
	for (uint8_t i = 0; i < 16; i++) {
		_nodes[i].used = false;
	}

	_encoder_btn.begin(PROGRAMMER_ENC_BTN, BUTTON_LONG_PUSH_TIME);
	pinMode(PROGRAMMER_ENC_A, INPUT_PULLUP);
	pinMode(PROGRAMMER_ENC_B, INPUT_PULLUP);

	can.register_filter(0x000, 0x000); // accept all frames in analysis mode

	_oled_time.begin(80);
	_ping_time.begin(400);

	_start_ap();

	#ifdef DEBUG
		Serial.println("> OLED + encoder + WiFi AP ready");
		Serial.print("> AP SSID ");
		Serial.println(PROGRAMMER_WIFI_SSID);
	#endif
}


void MODULE_PROGRAMMER::update(CAN_MESSAGE message) {

	if (message.uuid != 0 || message.id != 0 || message.size != 0) {
		_ingest(message);
	}

	_poll_encoder();

	if (_oled_time.check()) {
		_oled_time.retrigger();
		_draw_oled();
	}
}


void MODULE_PROGRAMMER::_ingest(CAN_MESSAGE message) {

	int8_t slot = _find_group(message.id, message.uuid);

	if (slot < 0) {
		for (uint8_t i = 0; i < PROGRAMMER_GROUP_MAX; i++) {
			if (!_groups[i].used) {
				slot = i;
				_groups[i].used = true;
				_groups[i].count = 0;
				break;
			}
		}
	}

	if (slot < 0) {
		return;
	}

	_groups[slot].id = message.id;
	_groups[slot].uuid = message.uuid;
	_groups[slot].last = message;
	_groups[slot].count++;
	_groups[slot].last_ms = millis();

	/* learn module presence from heartbeat and info replies */
	if (message.id == CAN_ID_MODULE_HEARTBEAT || message.id == CAN_ID_REPLY) {
		for (uint8_t i = 0; i < 16; i++) {
			if (_nodes[i].used && _nodes[i].uuid == message.uuid) {
				break;
			}
			if (!_nodes[i].used) {
				_nodes[i].used = true;
				_nodes[i].uuid = message.uuid;
				_nodes[i].type = message.size > 0 ? message.data[0] : 0;
				_nodes[i].name[0] = 0;
				break;
			}
		}
	}
}


int8_t MODULE_PROGRAMMER::_find_group(uint16_t id, uint16_t uuid) {

	for (uint8_t i = 0; i < PROGRAMMER_GROUP_MAX; i++) {
		if (_groups[i].used && _groups[i].id == id && _groups[i].uuid == uuid) {
			return i;
		}
	}
	return -1;
}


void MODULE_PROGRAMMER::_poll_encoder(void) {

	static int last_a = HIGH;
	int a = digitalRead(PROGRAMMER_ENC_A);

	if (a != last_a && a == LOW) {
		if (digitalRead(PROGRAMMER_ENC_B) == LOW) {
			_cursor++;
		} else {
			_cursor--;
		}
	}
	last_a = a;

	if (_encoder_btn.check() == 1) {
		_mode = (_mode == PROGRAMMER_MODE_ANALYSE)
			? PROGRAMMER_MODE_SETUP
			: PROGRAMMER_MODE_ANALYSE;
		_cursor = 0;
		if (_mode == PROGRAMMER_MODE_SETUP) {
			_ping();
		}
	}
}


void MODULE_PROGRAMMER::_ping(void) {

	CAN_MESSAGE request;
	request.id = CAN_ID_REQUEST;
	request.size = 1;
	request.data[0] = 0xFF;
	can.send(request);
}


void MODULE_PROGRAMMER::_draw_oled(void) {

	/*
	 * Host the colour list on the ST7789. The web UI in this repository
	 * is the full rendering of the same group table. On the device, draw
	 * PROGRAMMER_OLED_ROWS lines of name + CAN-ID + UUID.
	 */
}


void MODULE_PROGRAMMER::_start_ap(void) {

	/*
	 * WiFi.softAP(PROGRAMMER_WIFI_SSID, PROGRAMMER_WIFI_PASS);
	 * Serve the programmer webservice on PROGRAMMER_HTTP_PORT.
	 * Static export of the Next.js app can be stored in LittleFS / SPIFFS.
	 */
}

#endif
