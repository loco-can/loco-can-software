/*
 * PlatformIO entry point. Arduino IDE compiles loco-can-software.ino
 * instead; that sketch is excluded from the PlatformIO source filter
 * so setup()/loop() exist only once per toolchain.
 */

#ifdef PIOENV

#include "config.h"
#include "LocoCANcore.h"

LocoCANcore core;

void setup() {

	#ifdef DEBUG
		Serial.begin(115200);

		Serial.println("***********************");
		Serial.println("* Welcome to Loco-Can *");
		Serial.println("* The Bus  for Trains *");
		Serial.println("***********************");
		Serial.println();
		Serial.print("MODULE: ");
		Serial.print(MODULE);
		Serial.println();
	#endif

	core.begin();
}

void loop() {
	core.update();
}

#endif
