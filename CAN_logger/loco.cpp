#include "loco.h"


void LOCO::begin(OLED_DISPLAY* display) {
	_display = display;
	testval = 0;

	_battery_voltage.begin("Bat", display, 0, 30, 25000, BLUE);
	_motor_voltage.begin("Mot", display, 20, 30, 25000, BLUE);
	_current.begin("Amp", display, 40, 30, 65000, RED);

	_drive.begin("Fah", display, 68, 30, 1000, GREEN);
	// _drive.height(60);
	_break.begin("Bre", display, 88, 30, 1000, RED);
	// _break.height(60);

	_break.begin("Ges", display, 108, 30, 15000, CYAN);
}


void LOCO::update(CAN_MESSAGE* message) {

	_battery_voltage.update(testval);
	_motor_voltage.update(8000);
	_current.update(13000);

	_drive.update(500);

	if (testval > 25000) {
		testval = 0;
	}

	else {
		testval += 100;
	}

	// Serial.println(testval);
}