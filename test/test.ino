#include <Arduino.h>
#include "serialcan.h"
#include "can_com.h"


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