/*
 * Loco-CAN programmer function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2026
 * @lizence: GG0
 */

#include "../../config.h"

#ifdef MODULE_PROGRAMMER_CONFIG_H

#include "main.h"
#include <string.h>

extern CAN_COM can;

#ifdef MODULE_ARCH_ESP32
static MODULE_PROGRAMMER *programmer_web = nullptr;

static const char PROGRAMMER_FALLBACK_HTML[] PROGMEM =
	"<!doctype html><html lang=\"en\"><meta charset=\"utf-8\">"
	"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
	"<title>Loco-CAN Programmer</title>"
	"<body style=\"font-family:sans-serif;background:#16110c;color:#f3ead8;"
	"max-width:40rem;margin:2rem auto;padding:0 1.25rem\">"
	"<p style=\"letter-spacing:.28em;text-transform:uppercase;font-size:.7rem;"
	"color:#c4a36a\">Garden railway · 500 kbit/s</p>"
	"<h1>Loco-CAN Programmer</h1>"
	"<p>The access point is up, but the webservice is not on this ESP32 yet.</p>"
	"<p>On the development machine run <code>npm run firmware</code>, copy the "
	"<code>data/</code> folder next to <code>loco-can-software.ino</code>, "
	"upload LittleFS, then reboot.</p>"
	"<p>SSID <b>LOCO-CAN-PRG</b> · open <b>http://192.168.4.1</b></p>"
	"</body></html>";

static const char *programmer_mime(const String &path) {
	if (path.endsWith(".html")) return "text/html";
	if (path.endsWith(".js")) return "application/javascript";
	if (path.endsWith(".css")) return "text/css";
	if (path.endsWith(".woff2")) return "font/woff2";
	if (path.endsWith(".woff")) return "font/woff";
	if (path.endsWith(".svg")) return "image/svg+xml";
	if (path.endsWith(".png")) return "image/png";
	if (path.endsWith(".ico")) return "image/x-icon";
	if (path.endsWith(".json")) return "application/json";
	if (path.endsWith(".txt")) return "text/plain";
	return "application/octet-stream";
}

static int programmer_hex_nibble(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

static bool programmer_json_u32(const String &body, const char *key, uint32_t *out) {
	String needle = String("\"") + key + "\":";
	int i = body.indexOf(needle);
	if (i < 0) return false;
	*out = (uint32_t) body.substring(i + needle.length()).toInt();
	return true;
}
#endif


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

	#ifdef MODULE_ARCH_ESP32
		_fs_ok = false;
		_web_seq = 0;
		_web_head = 0;
		_web_count = 0;
	#endif

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
		#ifdef MODULE_ARCH_ESP32
			_web_push(message, false);
		#endif
	}

	_poll_encoder();

	#ifdef MODULE_ARCH_ESP32
		_web_poll();
	#endif

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
	memset(&request, 0, sizeof(request));
	request.id = CAN_ID_REQUEST;
	request.uuid = _own_uuid();
	request.size = 1;
	request.data[0] = 0xFF;
	can.send(request);
	#ifdef MODULE_ARCH_ESP32
		_web_push(request, true);
	#endif
}


void MODULE_PROGRAMMER::_draw_oled(void) {

	/*
	 * Host the colour list on the ST7789. The web UI in this repository
	 * is the full rendering of the same group table. On the device, draw
	 * PROGRAMMER_OLED_ROWS lines of name + CAN-ID + UUID.
	 */
}


uint16_t MODULE_PROGRAMMER::_own_uuid(void) {
	return (uint16_t) can.uuid();
}


void MODULE_PROGRAMMER::_start_ap(void) {

#ifdef MODULE_ARCH_ESP32
	programmer_web = this;

	WiFi.persistent(false);
	WiFi.mode(WIFI_AP);
	WiFi.softAPConfig(
		IPAddress(192, 168, 4, 1),
		IPAddress(192, 168, 4, 1),
		IPAddress(255, 255, 255, 0)
	);
	WiFi.softAP(PROGRAMMER_WIFI_SSID, PROGRAMMER_WIFI_PASS);

	_dns.start(53, "*", WiFi.softAPIP());

	_fs_ok = LittleFS.begin(true);

	_http.on("/api/status", HTTP_GET, []() {
		if (programmer_web) programmer_web->_http_status();
	});
	_http.on("/api/frames", HTTP_GET, []() {
		if (programmer_web) programmer_web->_http_frames();
	});
	_http.on("/api/tx", HTTP_POST, []() {
		if (programmer_web) programmer_web->_http_tx();
	});
	_http.onNotFound([]() {
		if (programmer_web) programmer_web->_http_static();
	});
	_http.begin();

	#ifdef DEBUG
		Serial.print("> AP IP ");
		Serial.println(WiFi.softAPIP());
		Serial.print("> LittleFS ");
		Serial.println(_fs_ok ? "mounted" : "missing");
	#endif
#endif
}


#ifdef MODULE_ARCH_ESP32

void MODULE_PROGRAMMER::_web_poll(void) {
	_dns.processNextRequest();
	_http.handleClient();
}


void MODULE_PROGRAMMER::_web_push(const CAN_MESSAGE &message, bool tx) {

	PROGRAMMER_WEB_FRAME &slot = _web_log[_web_head];
	slot.seq = ++_web_seq;
	slot.t = millis();
	slot.id = message.id;
	slot.uuid = message.uuid;
	slot.size = message.size > 8 ? 8 : message.size;
	slot.tx = tx ? 1 : 0;
	memcpy(slot.data, message.data, 8);
	_web_head = (_web_head + 1) % PROGRAMMER_WEB_FRAMES;
	if (_web_count < PROGRAMMER_WEB_FRAMES) {
		_web_count++;
	}
}


void MODULE_PROGRAMMER::_http_status(void) {

	IPAddress ip = WiFi.softAPIP();
	String body;
	body.reserve(160);
	body += "{\"ok\":true,\"ssid\":\"";
	body += PROGRAMMER_WIFI_SSID;
	body += "\",\"ip\":\"";
	body += ip.toString();
	body += "\",\"port\":";
	body += String(PROGRAMMER_HTTP_PORT);
	body += ",\"uuid\":";
	body += String(_own_uuid());
	body += ",\"mode\":";
	body += String(_mode);
	body += ",\"frames\":";
	body += String(_web_seq);
	body += "}";
	_http.sendHeader("Cache-Control", "no-store");
	_http.send(200, "application/json", body);
}


void MODULE_PROGRAMMER::_http_frames(void) {

	uint32_t after = 0;
	if (_http.hasArg("after")) {
		after = (uint32_t) _http.arg("after").toInt();
	}
	if (after > _web_seq) {
		after = 0;
	}

	String body;
	body.reserve(48 + _web_count * 48);
	body += "{\"seq\":";
	body += String(_web_seq);
	body += ",\"frames\":[";

	uint8_t start = _web_count < PROGRAMMER_WEB_FRAMES ? 0 : _web_head;
	bool first = true;
	uint8_t emitted = 0;

	for (uint8_t i = 0; i < _web_count && emitted < 32; i++) {
		const PROGRAMMER_WEB_FRAME &frame = _web_log[(start + i) % PROGRAMMER_WEB_FRAMES];
		if (frame.seq <= after) {
			continue;
		}
		if (!first) {
			body += ",";
		}
		first = false;
		body += "{\"t\":";
		body += String(frame.t);
		body += ",\"id\":";
		body += String(frame.id);
		body += ",\"u\":";
		body += String(frame.uuid);
		body += ",\"tx\":";
		body += String(frame.tx);
		body += ",\"d\":\"";
		for (uint8_t b = 0; b < frame.size; b++) {
			if (frame.data[b] < 0x10) body += "0";
			body += String(frame.data[b], HEX);
		}
		body += "\"}";
		emitted++;
	}

	body += "]}";
	_http.sendHeader("Cache-Control", "no-store");
	_http.send(200, "application/json", body);
}


void MODULE_PROGRAMMER::_http_tx(void) {

	String body = _http.arg("plain");
	if (body.length() == 0) {
		body = _http.arg("id").length()
			? String("{\"id\":") + _http.arg("id") + ",\"u\":" + _http.arg("u") + ",\"d\":\"" + _http.arg("d") + "\"}"
			: String();
	}

	CAN_MESSAGE message;
	memset(&message, 0, sizeof(message));

	uint32_t id = 0;
	uint32_t uuid = _own_uuid();
	if (!programmer_json_u32(body, "id", &id)) {
		_http.send(400, "application/json", "{\"ok\":false}");
		return;
	}
	programmer_json_u32(body, "u", &uuid);
	message.id = id;
	message.uuid = (uint16_t) uuid;

	int dpos = body.indexOf("\"d\":\"");
	if (dpos >= 0) {
		dpos += 5;
		while (message.size < 8 && dpos + 1 < (int) body.length()) {
			int hi = programmer_hex_nibble(body.charAt(dpos));
			int lo = programmer_hex_nibble(body.charAt(dpos + 1));
			if (hi < 0 || lo < 0) {
				break;
			}
			message.data[message.size++] = (uint8_t) ((hi << 4) | lo);
			dpos += 2;
		}
	}

	can.send(message);
	_ingest(message);
	_web_push(message, true);
	_http.send(200, "application/json", "{\"ok\":true}");
}


void MODULE_PROGRAMMER::_http_static(void) {

	String uri = _http.uri();
	if (uri.indexOf("..") >= 0) {
		_http.send(400, "text/plain", "bad path");
		return;
	}

	if (uri == "/generate_204" || uri == "/hotspot-detect.html" || uri == "/ncsi.txt" || uri == "/connecttest.txt") {
		_http.sendHeader("Location", "http://192.168.4.1/");
		_http.send(302, "text/plain", "");
		return;
	}

	if (uri.startsWith("/api/")) {
		_http.send(404, "application/json", "{\"ok\":false}");
		return;
	}

	if (_http_send_file(uri)) {
		return;
	}
	if (uri != "/" && _http_send_file("/index.html")) {
		return;
	}

	_http.sendHeader("Cache-Control", "no-store");
	_http.send_P(200, "text/html", PROGRAMMER_FALLBACK_HTML);
}


bool MODULE_PROGRAMMER::_http_send_file(const String &uri) {

	if (!_fs_ok) {
		return false;
	}

	String path = uri;
	if (path.length() == 0 || path == "/") {
		path = "/index.html";
	} else if (path.endsWith("/")) {
		path += "index.html";
	}

	String gz = path + ".gz";
	String open_path;
	bool gzip = false;

	if (LittleFS.exists(gz)) {
		open_path = gz;
		gzip = true;
	} else if (LittleFS.exists(path)) {
		open_path = path;
	} else {
		return false;
	}

	File file = LittleFS.open(open_path, "r");
	if (!file) {
		return false;
	}

	if (gzip) {
		_http.sendHeader("Content-Encoding", "gzip");
	}
	if (path.startsWith("/_next/")) {
		_http.sendHeader("Cache-Control", "public, max-age=31536000, immutable");
	} else {
		_http.sendHeader("Cache-Control", "no-cache");
	}
	_http.streamFile(file, programmer_mime(path));
	file.close();
	return true;
}

#endif

#endif
