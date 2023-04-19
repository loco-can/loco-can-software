#include <Arduino.h>
#include "serialcan.h"
#include "can_com.h"

/*
 * ATTiny85 pinout
 *        ADC0   /RESET 5/A0    PCINT5  PB5   1   8   VCC
 * XTAL1  ADC3          3/A3    PCINT3  PB3   2   7   PB2   PCINT2  2/A1  SCK   SCL   INT0  ADC1
 * XTAL2  ADC2    OC1B  4/A2    PCINT4  PB4   3   6   PB1   PCINT1  1     MISO  DO    OCB0B OC1A  AIN1
 *                                      GND   4   5   PB0   PCINT0  0     MOSI  DI    SDA   OC0A  AIN0   AREF
 */


SERIALCAN s_can;
CAN_MESSAGE message;

void setup(void) {

	s_can.begin(115200);

	message.size = 4;
	message.id = 0x12345678;
	message.uuid = 0xfedc;

	message.data[0] = 0x11;
	message.data[1] = 0x22;
	message.data[2] = 0x33;
	message.data[3] = 0x00;
	message.data[4] = 0x55;
	message.data[5] = 0x66;
	message.data[6] = 0x77;
	message.data[7] = 0x88;
}


void loop(void) {

	// read from serial
	if (s_can.available()) {
		message = s_can.read();
	}

	s_can.send(&message);

	delay(500);
}