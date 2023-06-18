//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>

// #ifdef ESP32
//     #include <SPIFFS.h>
//     #include <WiFi.h>
//     #include <AsyncTCP.h>
// #elif defined(ESP8266)

// #include <ESP8266WiFi.h>
// #include <ESPAsyncTCP.h>
// #endif

// #include <ESPAsyncWebServer.h>

// #include <ArduinoJson.h>
#include "config.h"
#include "module_board.h"

#include "intelliLed.h"
#include "can_protocol.h"
#include "loco_config.h"

#include "webserver.h"
#include "can_com.h"



struct DATA {
	double uuid;
	int type;
};


INTELLILED wifi_led(WIFI_STATUS_LED);
WEBSERVER server(WIFI_PORT);
CAN_COM can_com(CAN_RX, CAN_TX);
CAN_MESSAGE message;


void setup() {

	wifi_led.flash(1000);

	// while(!Serial) {
	// 	wifi_led.update();
	// }

	#ifdef DEBUG
		Serial.begin(115200);
		Serial.println("");
	#endif

	// disable watchdogs
	disableCore0WDT();
	disableCore1WDT();
	disableLoopWDT();

	wifi_led.on();

	// ===================================
	// select WiFi connection type
	#ifdef DEBUG
		Serial.println("Start WiFi module");
	#endif

	if (WIFI_AP_MODE == true) {
		server.ap((char*)AP_SSID, (char*)AP_PASSWORD, wifi_led);
	}
	else {
		server.wifi((char*)WIFI_SSID, (char*)WIFI_PASSWORD, wifi_led);
	}

	server.begin();
	wifi_led.off();


	// ===================================
	// CAN initialisation
    can_com.begin(500E3, CAN_STATUS_LED);

	wifi_led.flash(500);
}


void loop() {

	uint16_t filter;

	if (filter = can_com.read(&message)) {
		Serial.println(filter);
	}


	// blink heartbeat
	wifi_led.update();
}
