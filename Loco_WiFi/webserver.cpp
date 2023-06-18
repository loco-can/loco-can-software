/*
 * LOCO-CAN Webserver

 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */


#include "config.h"
#include "can_protocol.h"

#include "intelliLed.h"

#include "webserver.h"


// init webserver on port
WEBSERVER::WEBSERVER(uint16_t port) {
	_server = new AsyncWebServer(port);
	_port = port;
}


// connect to wifi
void WEBSERVER::wifi(char* ssid, char* password, INTELLILED wifi_led) {

	_wifi_led = wifi_led;
	_wifi_led.flash(WIFI_LED_CONNECT_FLASH);

	_connect(false, ssid, password);

	_wifi_led.off();
}


// create access point
void WEBSERVER::ap(char* ssid, char* password, INTELLILED wifi_led) {

	_wifi_led = wifi_led;
	_wifi_led.flash(WIFI_LED_CONNECT_FLASH);

	_connect(true, ssid, password);

	_wifi_led.off();
}


// connect to wifi / create access point
void WEBSERVER::_connect(bool ap, char* ssid, char* password) {

	disconnect();

	_ap = ap;
	_ssid = ssid;
	_password = password;
}


// disconnect wifi
void WEBSERVER::disconnect(void) {
	WiFi.disconnect();

	#ifdef DEBUG
		Serial.println("WiFi disconnected");
		Serial.println();
	#endif
}


// begin webserver
void WEBSERVER::begin() {


	// create ACCESS POINT
	if (_ap == true) {

		#ifdef DEBUG
			Serial.println("Configuring access point");
		#endif

		WiFi.softAP(_ssid, _password);
		// WiFi.begin();  

		_ip = WiFi.softAPIP();

		#ifdef DEBUG
			Serial.println("Server started");
			Serial.print("Wifi access point IP ");
		#endif
	}

	// use WIFI client mode
	else {

		#ifdef DEBUG
			Serial.print("Connect to WiFi ");
			Serial.print(_ssid);
			Serial.print(" ");
		#endif

		WiFi.begin(_ssid, _password);

		// wait for connection
		while (WiFi.status() != WL_CONNECTED) {

			_wifi_led.on();
			delay(20);
			_wifi_led.off();

			#ifdef DEBUG
				Serial.print(".");
			#endif

			delay(WIFI_CONNECT_WAIT);
		}

		#ifdef DEBUG
			Serial.println();
		#endif

		_ip = WiFi.localIP();
	}

	#ifdef DEBUG
		Serial.print("Create WebServer at ");
		Serial.print(_ip);
		Serial.print(":");
		Serial.println(_port);
		Serial.print("web status led port ");
		Serial.println(_wifi_led.port());

		Serial.println("start SPIFFS");
		Serial.println();
		Serial.println("WiFi starup OK!");
	#endif

	SPIFFS.begin();


	// =============================================
	// serve static pages
	_server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");


	// =============================================
	// administration page
	// _server->on("/admin", HTTP_GET, [this] (AsyncWebServerRequest *request) {
	//     request->send(200, "text/html", _ip.toString());
	// });


	// =============================================
	// GET API call
	_server->on("/api", HTTP_GET, [this] (AsyncWebServerRequest *request) {
	  
		String cmd;
		StaticJsonDocument<256> json;
		String output;

		_wifi_led.on();

		// int cnt = request->params();

		if (request->hasParam("cmd")) {

			cmd = request->getParam("cmd")->value();

			#ifdef DEBUG
				Serial.print("API access: ");
				Serial.println(cmd);
			#endif

			// ADD STATUS
			json["status"]["ip"] = _ip.toString();

			if (_ap) {
				json["status"]["type"] = "Access Point Mode";
			}

			else {
				json["status"]["type"] = "WiFi Client";
			}


			// =======================================
			// LOAD DATA
			if (cmd == "load") {
				json["data"][0]["type"] = "DRIVE";
				json["data"][0]["uuid"] = "2237498787";
				json["data"][0]["data"] = "0100010010";

				json["data"][1]["type"] = "LIGHT";
				json["data"][1]["uuid"] = "098745828";
				json["data"][1]["data"] = "00010010";
			}



		}
		
		else {
			json["error"] = "no command";
		}

		serializeJson(json, output);

		// request->addHeader("Access-Control-Allow-Origin", "*");
		request->send(200, "application/json", output);

		_wifi_led.off();
	});


	// =============================================
	// POST to API
	_server->on("/api", HTTP_POST, [this](AsyncWebServerRequest *request) {
	  
		String message;

		_wifi_led.on();

		int cnt = request->params();

		#ifdef DEBUG
			Serial.println("POST received");
			Serial.print("params ");
			Serial.println(cnt);
		#endif

		if (request->hasParam("cmd", true)) {
			message = request->getParam("cmd", true)->value();
		}
		
		else {
			message = "No message sent";
		}
		
		request->send(200, "text/plain", "POST: " + message + " with " + String(cnt) + " params");

		_wifi_led.off();
	});


	// =============================================
	// nothing found
	_server->onNotFound( [](AsyncWebServerRequest *request) {

		// _wifi_led.on();

		request->send(404, "text/plain", "page Not found");

		// _wifi_led.off();
	});


	_server->begin();
}


