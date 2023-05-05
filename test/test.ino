#include <Arduino.h>




void setup(void) {

  Serial.begin(115200);
	pinMode(A3, INPUT);

}


void loop(void) {

  int val;

  val = analogRead(A3);

  Serial.println(val);

	delay(500);


}
