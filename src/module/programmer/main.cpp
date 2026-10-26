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
#include "../../core/setup/setup_mode.h"
#include <string.h>

extern CAN_COM can;

#ifdef MODULE_ARCH_ESP32
#include "driver/twai.h"
#include "driver/gpio.h"
#include "soc/gpio_sig_map.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>

static MODULE_PROGRAMMER *programmer_web = nullptr;

class ProgrammerCatchAll : public RequestHandler {
	bool canHandle(HTTPMethod, String) override {
		return true;
	}
	bool handle(WebServer &, HTTPMethod, String) override {
		if (programmer_web) {
			programmer_web->_http_static();
		}
		return true;
	}
};

/*
 * Arduino-ESP32 LittleFS.exists()/open() on a missing path logs
 * vfs_api "does not exist, no permits for creation". Index the
 * image once and only open names that were listed.
 */
#define PROGRAMMER_FS_MAX 128
static String programmer_fs_paths[PROGRAMMER_FS_MAX];
static uint8_t programmer_fs_n = 0;

static String programmer_fs_normalize(const char *name) {
	if (!name || !name[0]) {
		return String();
	}
	if (name[0] == '/') {
		return String(name);
	}
	return String("/") + name;
}

static void programmer_fs_scan_dir(File dir) {
	File f = dir.openNextFile();
	while (f) {
		if (f.isDirectory()) {
			programmer_fs_scan_dir(f);
		} else if (programmer_fs_n < PROGRAMMER_FS_MAX) {
			const char *p = f.path();
			if (!p || !p[0]) {
				p = f.name();
			}
			String n = programmer_fs_normalize(p);
			if (n.length() > 0) {
				programmer_fs_paths[programmer_fs_n++] = n;
			}
		}
		f.close();
		f = dir.openNextFile();
	}
}

static bool programmer_fs_scan(void) {
	programmer_fs_n = 0;
	File root = LittleFS.open("/");
	if (!root) {
		return false;
	}
	programmer_fs_scan_dir(root);
	root.close();
	for (uint8_t i = 0; i < programmer_fs_n; i++) {
		if (programmer_fs_paths[i].endsWith("index.html") ||
			programmer_fs_paths[i].endsWith("index.html.gz")) {
			return true;
		}
	}
	return false;
}

static bool programmer_fs_known(const String &path) {
	for (uint8_t i = 0; i < programmer_fs_n; i++) {
		if (programmer_fs_paths[i] == path) {
			return true;
		}
	}
	return false;
}

static const char PROGRAMMER_FALLBACK_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Loco-CAN Programmer</title>
<style>
body{font-family:sans-serif;background:#16110c;color:#f3ead8;margin:1.25rem;max-width:48rem}
#st{color:#c4a36a}
#log{font:13px/1.45 ui-monospace,monospace;margin-top:1rem}
#log div{border-bottom:1px solid #3a3228;padding:.2rem 0}
.tx{color:#c4a36a}
</style>
<p>LOCO-CAN-PRG · <span id="st">connecting…</span></p>
<p>LittleFS UI is not installed. Frames are collected for 1 s and grouped by UUID.</p>
<div id="log"></div>
<script>
let after=0,lastPaint=0;
const live=new Map();
const order=[];
let view=[];
function hex(n){return '0x'+Number(n).toString(16);}
function render(){
  const el=document.getElementById('log');
  el.innerHTML=view.map(b=>
    '<div><strong>'+hex(b.u)+'</strong></div>'+
    b.msgs.map(f=>
      '<div class="'+(f.tx?'tx':'')+'">'+
      (f.tx?'TX ':'RX ')+hex(f.id)+' '+f.n+'/s '+(f.d||'')+'</div>'
    ).join('')
  ).join('');
}
function snapshot(now){
  live.forEach((f,k)=>{
    if(now-f.t>=10000) live.delete(k);
    else {if(f.w>0) f.n=f.w;f.w=0;}
  });
  const by=new Map();
  live.forEach(f=>{
    const list=by.get(f.u)||[];
    list.push(f);
    by.set(f.u,list);
    if(order.indexOf(f.u)<0) order.push(f.u);
  });
  view=order.filter(u=>by.has(u)).map(u=>({
    u,
    msgs:(by.get(u)||[]).slice().sort((a,b)=>a.id-b.id)
  }));
  for(let i=order.length-1;i>=0;i--) if(!by.has(order[i])) order.splice(i,1);
  render();
}
async function tick(){
  try{
    const s=await fetch('/api/status').then(r=>r.json());
    const j=await fetch('/api/frames?after='+after).then(r=>r.json());
    after=j.seq;
    (j.frames||[]).forEach(f=>{
      const k=f.id+':'+f.u;
      const g=live.get(k);
      live.set(k,{id:f.id,u:f.u,tx:f.tx,d:f.d||'',t:Date.now(),w:(g?g.w:0)+1,n:g?g.n:0});
    });
    const now=Date.now();
    if(!view.length && live.size){lastPaint=now;snapshot(now);}
    else if(now-lastPaint>=1000){lastPaint=now;snapshot(now);}
    document.getElementById('st').textContent=
      'uuid '+hex(s.uuid)+' · '+view.length+' UUID';
  }catch(e){
    document.getElementById('st').textContent='api error';
  }
}
setInterval(tick,400);
tick();
</script>
)HTML";

static const char *programmer_mime(const String &path) {
	if (path.endsWith(".html")) return "text/html; charset=utf-8";
	if (path.endsWith(".js")) return "text/javascript";
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

static Adafruit_SSD1351 programmer_oled(
	PROGRAMMER_OLED_WIDTH,
	PROGRAMMER_OLED_HEIGHT,
	&SPI,
	PROGRAMMER_OLED_CS,
	PROGRAMMER_OLED_DC,
	PROGRAMMER_OLED_RST
);

#define PROGRAMMER_OLED_BLACK 0x0000
#define PROGRAMMER_OLED_DIM 0x7BEF
#define PROGRAMMER_OLED_TEXT 0xFFFF
#define PROGRAMMER_OLED_ACCENT 0x07FF
#define PROGRAMMER_OLED_SEL 0xFFE0

static void programmer_hex4(char *out, uint16_t value) {
	static const char hex[] = "0123456789ABCDEF";
	out[0] = hex[(value >> 12) & 0xF];
	out[1] = hex[(value >> 8) & 0xF];
	out[2] = hex[(value >> 4) & 0xF];
	out[3] = hex[value & 0xF];
	out[4] = 0;
}

static const char *programmer_id_name(uint16_t id) {
	if (id >= CAN_ID_SETUP && id < CAN_ID_REPLY) {
		return "SET";
	}
	if (id > CAN_ID_REPLY && id < CAN_ID_REQUEST) {
		return "VAL";
	}
	switch (id) {
		case CAN_ID_EMERGENCY: return "ESTOP";
		case CAN_ID_DRIVE_HEARTBEAT: return "D-HB";
		case CAN_ID_TRAINEND_HEARTBEAT: return "END";
		case CAN_ID_MODULE_CURRENT: return "Imod";
		case CAN_ID_MOTOR_CURRENT: return "Imot";
		case CAN_ID_BATT_CURRENT: return "Ibat";
		case CAN_ID_SPEED: return "SPD";
		case CAN_ID_DIR: return "DIR";
		case CAN_ID_SIGNAL: return "SIG";
		case CAN_ID_TACHO: return "TACH";
		case CAN_ID_VOLTAGE: return "V";
		case CAN_ID_MOTOR_VOLTAGE: return "Vmot";
		case CAN_ID_BATT_VOLTAGE: return "Vbat";
		case CAN_ID_DRIVE: return "DRIVE";
		case CAN_ID_LIGHT: return "LIGHT";
		case CAN_ID_SWITCH: return "SW";
		case CAN_ID_VEHICLE_STATUS: return "STAT";
		case CAN_ID_MODULE_HEARTBEAT: return "HB";
		case CAN_ID_LOCO_SETUP: return "LOCO";
		case CAN_ID_REPLY: return "INFO";
		case CAN_ID_SETUP_MODE: return "SMODE";
		case CAN_ID_REQUEST: return "PING";
		default: return "CAN";
	}
}

static const char *programmer_type_name(uint8_t type) {
	switch (type) {
		case TYPE_CONTROLLER: return "CTRL";
		case TYPE_VEHICLE: return "VEH";
		case TYPE_SERVO: return "SRV";
		case TYPE_SWITCH: return "SW";
		case TYPE_DRIVE: return "DRV";
		case TYPE_SENSOR: return "SNS";
		case TYPE_PROGRAMMER: return "PRG";
		case TYPE_WIFI: return "WIFI";
		default: return "MOD";
	}
}

static void programmer_oled_begin(void) {
	SPI.begin(PROGRAMMER_OLED_SCK, -1, PROGRAMMER_OLED_MOSI, PROGRAMMER_OLED_CS);
	programmer_oled.begin();
	programmer_oled.fillScreen(PROGRAMMER_OLED_BLACK);
	programmer_oled.setTextWrap(false);
	programmer_oled.setTextSize(1);
	programmer_oled.setTextColor(PROGRAMMER_OLED_TEXT);
	programmer_oled.setCursor(4, 8);
	programmer_oled.print("Loco-CAN");
	programmer_oled.setCursor(4, 24);
	programmer_oled.print("Programmer");
}

static void programmer_oled_line(
	uint8_t row,
	bool selected,
	const char *left,
	const char *right
) {
	const int16_t y = 16 + (int16_t)row * 16;
	programmer_oled.fillRect(0, y, PROGRAMMER_OLED_WIDTH, 16, PROGRAMMER_OLED_BLACK);
	programmer_oled.setTextColor(selected ? PROGRAMMER_OLED_SEL : PROGRAMMER_OLED_DIM);
	programmer_oled.setCursor(4, y + 4);
	programmer_oled.print(left);
	if (right && right[0]) {
		programmer_oled.setCursor(76, y + 4);
		programmer_oled.print(right);
	}
}
#endif


void MODULE_PROGRAMMER::begin(void) {

	#ifdef DEBUG
		Serial.println("*************************");
		Serial.println("start function/programmer");
	#endif

	_mode = PROGRAMMER_MODE_ANALYSE;
	_cursor = 0;
	_setup_bus = false;

	for (uint8_t i = 0; i < PROGRAMMER_GROUP_MAX; i++) {
		_groups[i].used = false;
	}
	for (uint8_t i = 0; i < 16; i++) {
		_nodes[i].used = false;
	}

	#ifdef MODULE_ARCH_ESP32
		_fs_ok = false;
		_fs_ui = false;
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
	_setup_keep.begin(CAN_SETUP_MODE_KEEPALIVE_MS);

	#ifdef MODULE_ARCH_ESP32
		programmer_oled_begin();
	#endif

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

	if (_mode == PROGRAMMER_MODE_SETUP && _setup_bus && _setup_keep.update()) {
		_enter_bus_setup();
	}

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

	if (message.id == CAN_ID_DRIVE_HEARTBEAT) {
		loco_setup_note_controller(loco_setup, millis());
		if (_setup_bus) {
			_leave_bus_setup();
		}
	}

	if (message.id == CAN_ID_SETUP_MODE && message.size >= 3) {
		for (uint8_t i = 0; i < 16; i++) {
			bool known = _nodes[i].used && _nodes[i].uuid == message.uuid;
			if (!known && _nodes[i].used) {
				continue;
			}
			if (!known) {
				_nodes[i].used = true;
				_nodes[i].uuid = message.uuid;
				_nodes[i].name[0] = 0;
				_nodes[i].max_settings = 0;
			}
			_nodes[i].type = message.data[1];
			_nodes[i].version_maj = message.size > 3 ? message.data[3] : 0;
			_nodes[i].version_min = message.size > 4 ? message.data[4] : 0;
			_nodes[i].version_patch = message.size > 5 ? message.data[5] : 0;
			break;
		}
	}

	/* learn module presence from heartbeat and info replies */
	if (message.id == CAN_ID_MODULE_HEARTBEAT || message.id == CAN_ID_REPLY) {
		for (uint8_t i = 0; i < 16; i++) {
			bool known = _nodes[i].used && _nodes[i].uuid == message.uuid;
			if (!known && _nodes[i].used) {
				continue;
			}
			if (!known) {
				_nodes[i].used = true;
				_nodes[i].uuid = message.uuid;
				_nodes[i].name[0] = 0;
				_nodes[i].type = 0;
				_nodes[i].version_maj = 0;
				_nodes[i].version_min = 0;
				_nodes[i].version_patch = 0;
			}
			if (message.id == CAN_ID_MODULE_HEARTBEAT) {
				_nodes[i].type = message.size > 0 ? message.data[0] : 0;
				_nodes[i].version_maj = message.size > 1 ? message.data[1] : 0;
				_nodes[i].version_min = message.size > 2 ? message.data[2] : 0;
				_nodes[i].version_patch = message.size > 3 ? message.data[3] : 0;
			} else if (!known) {
				_nodes[i].type = message.size > 0 ? message.data[0] : 0;
				_nodes[i].version_maj = message.size > 1 ? message.data[1] : 0;
			}
			break;
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
		} else {
			_leave_bus_setup();
		}
	}
}


bool MODULE_PROGRAMMER::_controller_present(void) {
	return loco_setup_controller_present(loco_setup, millis());
}


bool MODULE_PROGRAMMER::_enter_bus_setup(void) {

	if (_controller_present()) {
		#ifdef DEBUG
			Serial.println("> setup blocked: controller heartbeat");
		#endif
		return false;
	}

	CAN_MESSAGE request;
	memset(&request, 0, sizeof(request));
	request.id = CAN_ID_REQUEST;
	request.uuid = _own_uuid();
	request.size = 1;
	request.data[0] = CAN_SETUP_MODE_ENTER;
	if (!can.send(request)) {
		return false;
	}
	_setup_bus = true;
	_setup_keep.retrigger();
#ifdef MODULE_ARCH_ESP32
	_web_push(request, true);
#endif
	return true;
}


void MODULE_PROGRAMMER::_leave_bus_setup(void) {

	if (!_setup_bus) {
		return;
	}

	CAN_MESSAGE request;
	memset(&request, 0, sizeof(request));
	request.id = CAN_ID_REQUEST;
	request.uuid = _own_uuid();
	request.size = 1;
	request.data[0] = CAN_SETUP_MODE_LEAVE;
	can.send(request);
	_setup_bus = false;
#ifdef MODULE_ARCH_ESP32
	_web_push(request, true);
#endif
}


void MODULE_PROGRAMMER::_ping(void) {

	if (!_enter_bus_setup()) {
		return;
	}

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

#ifdef MODULE_ARCH_ESP32
	programmer_oled.fillRect(0, 0, PROGRAMMER_OLED_WIDTH, 16, PROGRAMMER_OLED_BLACK);
	programmer_oled.setTextColor(PROGRAMMER_OLED_ACCENT);
	programmer_oled.setCursor(4, 4);
	programmer_oled.print(_mode == PROGRAMMER_MODE_SETUP ? "Setup" : "Analyse");

	uint8_t count = 0;
	if (_mode == PROGRAMMER_MODE_SETUP) {
		for (uint8_t i = 0; i < 16; i++) {
			if (_nodes[i].used) {
				count++;
			}
		}
	} else {
		for (uint8_t i = 0; i < PROGRAMMER_GROUP_MAX; i++) {
			if (_groups[i].used) {
				count++;
			}
		}
	}

	if (count == 0) {
		const char *empty = "Waiting CAN";
		if (_mode == PROGRAMMER_MODE_SETUP) {
			empty = _controller_present() ? "Controller HB" : "Enter setup";
		}
		programmer_oled_line(0, false, empty, "");
		for (uint8_t row = 1; row < PROGRAMMER_OLED_ROWS; row++) {
			programmer_oled.fillRect(0, 16 + (int16_t)row * 16, PROGRAMMER_OLED_WIDTH, 16, PROGRAMMER_OLED_BLACK);
		}
		return;
	}

	if (_cursor < 0) {
		_cursor = 0;
	}
	if (_cursor >= count) {
		_cursor = count - 1;
	}

	uint8_t start = 0;
	if (_cursor >= PROGRAMMER_OLED_ROWS) {
		start = (uint8_t)(_cursor - PROGRAMMER_OLED_ROWS + 1);
	}

	uint8_t index = 0;
	uint8_t drawn = 0;
	char left[18];
	char right[12];

	if (_mode == PROGRAMMER_MODE_SETUP) {
		for (uint8_t i = 0; i < 16 && drawn < PROGRAMMER_OLED_ROWS; i++) {
			if (!_nodes[i].used) {
				continue;
			}
			if (index < start) {
				index++;
				continue;
			}
			if (_nodes[i].name[0]) {
				strncpy(left, _nodes[i].name, 10);
				left[10] = 0;
			} else {
				strncpy(left, programmer_type_name(_nodes[i].type), sizeof(left) - 1);
				left[sizeof(left) - 1] = 0;
			}
			right[0] = '0';
			right[1] = 'x';
			programmer_hex4(right + 2, _nodes[i].uuid);
			programmer_oled_line(drawn, index == _cursor, left, right);
			index++;
			drawn++;
		}
	} else {
		for (uint8_t i = 0; i < PROGRAMMER_GROUP_MAX && drawn < PROGRAMMER_OLED_ROWS; i++) {
			if (!_groups[i].used) {
				continue;
			}
			if (index < start) {
				index++;
				continue;
			}
			strncpy(left, programmer_id_name(_groups[i].id), 8);
			left[8] = 0;
			right[0] = '0';
			right[1] = 'x';
			programmer_hex4(right + 2, _groups[i].uuid);
			programmer_oled_line(drawn, index == _cursor, left, right);
			index++;
			drawn++;
		}
	}

	while (drawn < PROGRAMMER_OLED_ROWS) {
		programmer_oled.fillRect(0, 16 + (int16_t)drawn * 16, PROGRAMMER_OLED_WIDTH, 16, PROGRAMMER_OLED_BLACK);
		drawn++;
	}
#endif
}


uint16_t MODULE_PROGRAMMER::_own_uuid(void) {
	return (uint16_t) can.uuid();
}


void MODULE_PROGRAMMER::_start_ap(void) {

#ifdef MODULE_ARCH_ESP32
	programmer_web = this;

	#ifdef DEBUG
		Serial.println("> starting WiFi AP");
		Serial.flush();
	#endif

	WiFi.persistent(false);
	WiFi.mode(WIFI_OFF);
	delay(50);
	WiFi.mode(WIFI_AP);
	WiFi.setSleep(false);
	delay(50);
	const bool ap_ok = WiFi.softAP(PROGRAMMER_WIFI_SSID, PROGRAMMER_WIFI_PASS, 1, 0, 4);
	WiFi.softAPConfig(
		IPAddress(192, 168, 4, 1),
		IPAddress(192, 168, 4, 1),
		IPAddress(255, 255, 255, 0)
	);
	delay(100);

	#ifdef DEBUG
		Serial.print("> AP ");
		Serial.println(ap_ok ? "ok" : "fail");
		Serial.print("> AP IP ");
		Serial.println(WiFi.softAPIP());
		Serial.flush();
	#endif

	_dns.start(53, "*", WiFi.softAPIP());

	/* format-on-fail would wipe a freshly uploaded LittleFS image */
	_fs_ok = LittleFS.begin(false);
	_fs_ui = _fs_ok && programmer_fs_scan();

	_http.on("/api/status", HTTP_GET, []() {
		if (programmer_web) programmer_web->_http_status();
	});
	_http.on("/api/frames", HTTP_GET, []() {
		if (programmer_web) programmer_web->_http_frames();
	});
	_http.on("/api/tx", HTTP_POST, []() {
		if (programmer_web) programmer_web->_http_tx();
	});
	_http.addHandler(new ProgrammerCatchAll());
	static const char *header_keys[] = { "Accept-Encoding" };
	_http.collectHeaders(header_keys, 1);
	_http.enableCORS(true);
	_http.begin();

	loco_twai_rebind_rx();

	#ifdef DEBUG
		Serial.print("> LittleFS ");
		if (_fs_ui) {
			Serial.println("UI ready");
		} else if (_fs_ok) {
			Serial.println("mounted, no UI (fallback page)");
		} else {
			Serial.println("missing");
		}
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
	body += ",\"setup\":";
	body += _setup_bus ? "true" : "false";
	body += ",\"controller\":";
	body += _controller_present() ? "true" : "false";
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

	for (uint8_t i = 0; i < _web_count && emitted < PROGRAMMER_WEB_FRAMES; i++) {
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

	if (loco_setup_is_enter(message.id, message.size, message.data[0])) {
		if (_controller_present()) {
			_http.send(503, "application/json", "{\"ok\":false,\"error\":\"Controller heartbeat on the bus\"}");
			return;
		}
		_setup_bus = true;
		_setup_keep.retrigger();
		_mode = PROGRAMMER_MODE_SETUP;
	} else if (loco_setup_is_leave(message.id, message.size, message.data[0])) {
		_setup_bus = false;
		_mode = PROGRAMMER_MODE_ANALYSE;
	}

	const bool sent = can.send(message);
	if (!sent) {
		_http.send(503, "application/json", "{\"ok\":false,\"error\":\"CAN TX failed\"}");
		return;
	}
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

	if (!_fs_ok || !_fs_ui) {
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
	const String accept = _http.header("Accept-Encoding");
	const bool want_gzip = accept.indexOf("gzip") >= 0;

	if (want_gzip && programmer_fs_known(gz)) {
		open_path = gz;
		gzip = true;
	} else if (programmer_fs_known(path)) {
		open_path = path;
	} else if (programmer_fs_known(gz)) {
		open_path = gz;
		gzip = true;
	} else {
		return false;
	}

	File file = LittleFS.open(open_path, "r");
	if (!file) {
		return false;
	}

	const size_t len = file.size();
	if (gzip) {
		_http.sendHeader("Content-Encoding", "gzip");
	}
	if (path.startsWith("/_next/")) {
		_http.sendHeader("Cache-Control", "public, max-age=31536000, immutable");
	} else {
		_http.sendHeader("Cache-Control", "no-cache");
	}
	_http.setContentLength(len);
	_http.send(200, programmer_mime(path), "");

	uint8_t buf[1024];
	size_t sent = 0;
	while (sent < len) {
		size_t chunk = len - sent;
		if (chunk > sizeof(buf)) {
			chunk = sizeof(buf);
		}
		int n = file.read(buf, chunk);
		if (n <= 0) {
			break;
		}
		_http.client().write(buf, (size_t) n);
		sent += (size_t) n;
	}
	file.close();
	return true;
}

#endif

#endif
