/*
 * Loco-CAN WIFI function
 *
 * @author: Thomas H Winkler
 * @copyright: 2026
 * @lizence: GG0
 *
 * FUNCTIONALITY
 *   Standard Loco-CAN node on the local TWAI bus.
 *   Discovers one peer WIFI module over ESP-NOW.
 *   Relays CAN frames to that peer only while the two modules are
 *   not on the same physical cable (detected by seeing the same
 *   frame on CAN and on ESP-NOW).
 */

#pragma once

#ifndef MODULE_WIFI_H
#define MODULE_WIFI_H

#include "../../config.h"
#include "../../core/can/can_com.h"
#include "../../can_protocol.h"
#include "../../core/timeout/intellitimeout.h"
#include "now_packet.h"

#ifdef MODULE_ARCH_ESP32
	#include <WiFi.h>
	#include <esp_now.h>
	#include <esp_wifi.h>
#endif

extern CAN_COM can;

struct WIFI_RECENT_FRAME {
	uint32_t id;
	uint16_t uuid;
	uint8_t size;
	uint8_t data[8];
	uint32_t t;
	bool used;
};

class MODULE_WIFI {

	public:
		void begin(void);
		void update(CAN_MESSAGE message);

#ifdef MODULE_ARCH_ESP32
		void _now_receive(const uint8_t *mac, const uint8_t *data, int len);
#endif

	private:
		uint8_t _mac[6];
		uint8_t _peer_mac[6];
		bool _peer_ok;
		bool _cable;
		uint32_t _peer_ms;
		uint32_t _cable_ms;
		uint8_t _seq;

		INTELLITIMEOUT _hello_time;
		INTELLITIMEOUT _heartbeat_time;

#ifdef MODULE_ARCH_ESP32
		WIFI_NOW_PACKET _rx_pkt[WIFI_NOW_QUEUE];
		uint8_t _rx_mac[WIFI_NOW_QUEUE][6];
		volatile uint8_t _rx_head;
		volatile uint8_t _rx_count;
#endif

		WIFI_RECENT_FRAME _recent_can[WIFI_RECENT_MAX];
		WIFI_RECENT_FRAME _recent_now[WIFI_RECENT_MAX];
		WIFI_RECENT_FRAME _injected[WIFI_RECENT_MAX];

		void _handle_can(CAN_MESSAGE message);
		void _poll_now(void);
		void _send_hello(void);
		void _send_module_heartbeat(void);
		void _handle_hello(const uint8_t *mac, const WIFI_NOW_PACKET &pkt);
		void _handle_now_can(const WIFI_NOW_PACKET &pkt);
		void _send_now_can(const CAN_MESSAGE &message);
		bool _add_peer(const uint8_t *mac);
		void _remember(WIFI_RECENT_FRAME *list, const CAN_MESSAGE &message);
		bool _seen(const WIFI_RECENT_FRAME *list, const CAN_MESSAGE &message, uint32_t window_ms);
		bool _same_mac(const uint8_t *a, const uint8_t *b);
		void _copy_mac(uint8_t *dst, const uint8_t *src);
};

#endif
