/*
 * Loco-CAN WIFI function
 *
 * @author: Thomas H Winkler
 * @copyright: 2026
 * @lizence: GG0
 */

#include "../../config.h"

#ifdef MODULE_WIFI_CONFIG_H

#include "main.h"
#include <string.h>

#ifdef MODULE_ARCH_ESP32

static MODULE_WIFI *wifi_mod = nullptr;
static portMUX_TYPE wifi_now_mux = portMUX_INITIALIZER_UNLOCKED;

static const uint8_t WIFI_BROADCAST_MAC[6] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)
static void wifi_now_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
	if (wifi_mod == nullptr || info == nullptr || info->src_addr == nullptr) {
		return;
	}
	wifi_mod->_now_receive(info->src_addr, data, len);
}
#else
static void wifi_now_recv(const uint8_t *mac, const uint8_t *data, int len) {
	if (wifi_mod == nullptr || mac == nullptr) {
		return;
	}
	wifi_mod->_now_receive(mac, data, len);
}
#endif

#endif


void MODULE_WIFI::begin(void) {

	#ifdef DEBUG
		Serial.println("*************************");
		Serial.println("start function/wifi");
	#endif

	memset(_mac, 0, sizeof(_mac));
	memset(_peer_mac, 0, sizeof(_peer_mac));
	_peer_ok = false;
	_cable = false;
	_peer_ms = 0;
	_cable_ms = 0;
	_seq = 0;

	for (uint8_t i = 0; i < WIFI_RECENT_MAX; i++) {
		_recent_can[i].used = false;
		_recent_now[i].used = false;
		_injected[i].used = false;
	}

#ifdef MODULE_ARCH_ESP32
	_rx_head = 0;
	_rx_count = 0;
	wifi_mod = this;

	WiFi.persistent(false);
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	WiFi.macAddress(_mac);

	esp_wifi_set_ps(WIFI_PS_NONE);
	esp_wifi_set_channel(WIFI_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

	if (esp_now_init() != ESP_OK) {
		#ifdef DEBUG
			Serial.println("*** ESP-NOW init failed");
		#endif
	}
	else {
		esp_now_register_recv_cb(wifi_now_recv);
		_add_peer(WIFI_BROADCAST_MAC);
	}

	#ifdef DEBUG
		Serial.print("> WIFI MAC ");
		Serial.println(WiFi.macAddress());
		Serial.print("> ESP-NOW channel ");
		Serial.println(WIFI_NOW_CHANNEL);
	#endif
#endif

	_hello_time.begin(WIFI_NOW_HELLO_MS);
	_heartbeat_time.begin(MODULE_HEARTBEAT_TIMEOUT);
}


void MODULE_WIFI::update(CAN_MESSAGE message) {

	if (message.id != 0 || message.uuid != 0 || message.size != 0) {
		_handle_can(message);
	}

	CAN_MESSAGE frame;
	while (can.fetch(frame)) {
		_handle_can(frame);
	}

	_poll_now();

	if (_peer_ok && (millis() - _peer_ms) > WIFI_NOW_PEER_TIMEOUT_MS) {
		_peer_ok = false;
		#ifdef DEBUG
			Serial.println("> WIFI peer lost");
		#endif
	}

	if (_cable && (millis() - _cable_ms) > WIFI_NOW_CABLE_TIMEOUT_MS) {
		_cable = false;
		#ifdef DEBUG
			Serial.println("> WIFI cable gone — bridging on");
		#endif
	}

	if (_hello_time.update()) {
		_send_hello();
	}

	if (_heartbeat_time.update()) {
		_send_module_heartbeat();
	}
}


void MODULE_WIFI::_handle_can(CAN_MESSAGE message) {

	if (_seen(_injected, message, 80)) {
		return;
	}

	if (_seen(_recent_now, message, WIFI_NOW_CABLE_TIMEOUT_MS)) {
		if (!_cable) {
			#ifdef DEBUG
				Serial.println("> WIFI cable present — bridging off");
			#endif
		}
		_cable = true;
		_cable_ms = millis();
	}

	_remember(_recent_can, message);

	if (_peer_ok) {
		_send_now_can(message);
	}
}


void MODULE_WIFI::_handle_now_can(const WIFI_NOW_PACKET &pkt) {

	CAN_MESSAGE message;
	memset(&message, 0, sizeof(message));
	message.id = pkt.can_id;
	message.uuid = pkt.can_uuid;
	message.size = pkt.size > 8 ? 8 : pkt.size;
	memcpy(message.data, pkt.data, message.size);

	if (_seen(_recent_can, message, WIFI_NOW_CABLE_TIMEOUT_MS)) {
		if (!_cable) {
			#ifdef DEBUG
				Serial.println("> WIFI cable present — bridging off");
			#endif
		}
		_cable = true;
		_cable_ms = millis();
		return;
	}

	if (_seen(_injected, message, 80) || _seen(_recent_now, message, 80)) {
		return;
	}

	_remember(_recent_now, message);

	if (!wifi_now_should_bridge(_peer_ok, _cable)) {
		return;
	}

	_remember(_injected, message);
	can.forward(message);
}


void MODULE_WIFI::_send_now_can(const CAN_MESSAGE &message) {

#ifdef MODULE_ARCH_ESP32
	if (!_peer_ok) {
		return;
	}

	WIFI_NOW_PACKET pkt;
	memset(&pkt, 0, sizeof(pkt));
	pkt.type = WIFI_NOW_CAN;
	pkt.seq = ++_seq;
	pkt.uuid = (uint16_t)can.uuid();
	pkt.can_id = message.id;
	pkt.can_uuid = message.uuid;
	pkt.size = message.size > 8 ? 8 : message.size;
	pkt.flags = _cable ? WIFI_NOW_FLAG_CABLE : 0;
	memcpy(pkt.data, message.data, pkt.size);

	uint8_t wire[WIFI_NOW_WIRE_SIZE];
	if (wifi_now_pack(&pkt, wire, sizeof(wire)) == 0) {
		return;
	}
	esp_now_send(_peer_mac, wire, WIFI_NOW_WIRE_SIZE);
#else
	(void)message;
#endif
}


void MODULE_WIFI::_send_hello(void) {

#ifdef MODULE_ARCH_ESP32
	WIFI_NOW_PACKET pkt;
	memset(&pkt, 0, sizeof(pkt));
	pkt.type = WIFI_NOW_HELLO;
	pkt.seq = ++_seq;
	pkt.uuid = (uint16_t)can.uuid();
	pkt.flags = _cable ? WIFI_NOW_FLAG_CABLE : 0;

	uint8_t wire[WIFI_NOW_WIRE_SIZE];
	if (wifi_now_pack(&pkt, wire, sizeof(wire)) == 0) {
		return;
	}

	esp_now_send((uint8_t *)WIFI_BROADCAST_MAC, wire, WIFI_NOW_WIRE_SIZE);
	if (_peer_ok) {
		esp_now_send(_peer_mac, wire, WIFI_NOW_WIRE_SIZE);
	}
#endif
}


void MODULE_WIFI::_send_module_heartbeat(void) {

	CAN_MESSAGE heartbeat;
	memset(&heartbeat, 0, sizeof(heartbeat));
	heartbeat.id = CAN_ID_MODULE_HEARTBEAT;
	heartbeat.size = 2;
	heartbeat.data[0] = WIFI_MODULE_TYPE;
	heartbeat.data[1] = WIFI_MODULE_VERSION;
	can.send(heartbeat);
}


void MODULE_WIFI::_handle_hello(const uint8_t *mac, const WIFI_NOW_PACKET &pkt) {

#ifdef MODULE_ARCH_ESP32
	if (_same_mac(mac, _mac)) {
		return;
	}

	if (_peer_ok && !_same_mac(mac, _peer_mac)) {
		return;
	}

	if (!_peer_ok) {
		if (!_add_peer(mac)) {
			return;
		}
		_copy_mac(_peer_mac, mac);
		_peer_ok = true;
		#ifdef DEBUG
			Serial.print("> WIFI peer ");
			for (uint8_t i = 0; i < 6; i++) {
				if (_peer_mac[i] < 0x10) {
					Serial.print("0");
				}
				Serial.print(_peer_mac[i], HEX);
				if (i < 5) {
					Serial.print(":");
				}
			}
			Serial.print(" uuid=");
			Serial.println(pkt.uuid, HEX);
		#endif
	}

	_peer_ms = millis();

	if (pkt.flags & WIFI_NOW_FLAG_CABLE) {
		_cable = true;
		_cable_ms = millis();
	}
#else
	(void)mac;
	(void)pkt;
#endif
}


void MODULE_WIFI::_poll_now(void) {

#ifdef MODULE_ARCH_ESP32
	for (;;) {
		WIFI_NOW_PACKET pkt;
		uint8_t mac[6];
		bool have = false;

		portENTER_CRITICAL(&wifi_now_mux);
		if (_rx_count > 0) {
			uint8_t start = (_rx_head + WIFI_NOW_QUEUE - _rx_count) % WIFI_NOW_QUEUE;
			pkt = _rx_pkt[start];
			_copy_mac(mac, _rx_mac[start]);
			_rx_count--;
			have = true;
		}
		portEXIT_CRITICAL(&wifi_now_mux);

		if (!have) {
			break;
		}

		if (pkt.type == WIFI_NOW_HELLO) {
			_handle_hello(mac, pkt);
		}
		else if (pkt.type == WIFI_NOW_CAN) {
			if (!_peer_ok) {
				_handle_hello(mac, pkt);
			}
			if (_peer_ok && _same_mac(mac, _peer_mac)) {
				_handle_now_can(pkt);
			}
		}
	}
#endif
}


#ifdef MODULE_ARCH_ESP32

void MODULE_WIFI::_now_receive(const uint8_t *mac, const uint8_t *data, int len) {

	if (mac == 0 || data == 0 || len < WIFI_NOW_WIRE_SIZE) {
		return;
	}

	WIFI_NOW_PACKET pkt;
	if (!wifi_now_unpack(data, (uint8_t)len, &pkt)) {
		return;
	}

	portENTER_CRITICAL(&wifi_now_mux);
	_rx_pkt[_rx_head] = pkt;
	_copy_mac(_rx_mac[_rx_head], mac);
	_rx_head = (_rx_head + 1) % WIFI_NOW_QUEUE;
	if (_rx_count < WIFI_NOW_QUEUE) {
		_rx_count++;
	}
	portEXIT_CRITICAL(&wifi_now_mux);
}

#endif


bool MODULE_WIFI::_add_peer(const uint8_t *mac) {

#ifdef MODULE_ARCH_ESP32
	if (esp_now_is_peer_exist(mac)) {
		return true;
	}

	esp_now_peer_info_t peer;
	memset(&peer, 0, sizeof(peer));
	_copy_mac(peer.peer_addr, mac);
	peer.channel = WIFI_NOW_CHANNEL;
	peer.encrypt = false;
	peer.ifidx = WIFI_IF_STA;
	return esp_now_add_peer(&peer) == ESP_OK;
#else
	(void)mac;
	return false;
#endif
}


void MODULE_WIFI::_remember(WIFI_RECENT_FRAME *list, const CAN_MESSAGE &message) {

	for (uint8_t i = 0; i < WIFI_RECENT_MAX; i++) {
		if (list[i].used
			&& wifi_now_same_frame(
				list[i].id, list[i].uuid, list[i].data, list[i].size,
				message.id, message.uuid, message.data, message.size
			)
		) {
			list[i].t = millis();
			return;
		}
	}

	uint8_t slot = 0;
	uint32_t oldest = 0xFFFFFFFF;
	for (uint8_t i = 0; i < WIFI_RECENT_MAX; i++) {
		if (!list[i].used) {
			slot = i;
			break;
		}
		if (list[i].t < oldest) {
			oldest = list[i].t;
			slot = i;
		}
	}

	list[slot].used = true;
	list[slot].id = message.id;
	list[slot].uuid = message.uuid;
	list[slot].size = message.size > 8 ? 8 : message.size;
	memset(list[slot].data, 0, 8);
	memcpy(list[slot].data, message.data, list[slot].size);
	list[slot].t = millis();
}


bool MODULE_WIFI::_seen(const WIFI_RECENT_FRAME *list, const CAN_MESSAGE &message, uint32_t window_ms) {

	uint32_t now = millis();
	for (uint8_t i = 0; i < WIFI_RECENT_MAX; i++) {
		if (!list[i].used) {
			continue;
		}
		if ((now - list[i].t) > window_ms) {
			continue;
		}
		if (wifi_now_same_frame(
			list[i].id, list[i].uuid, list[i].data, list[i].size,
			message.id, message.uuid, message.data, message.size
		)) {
			return true;
		}
	}
	return false;
}


bool MODULE_WIFI::_same_mac(const uint8_t *a, const uint8_t *b) {
	return memcmp(a, b, 6) == 0;
}


void MODULE_WIFI::_copy_mac(uint8_t *dst, const uint8_t *src) {
	memcpy(dst, src, 6);
}

#endif
