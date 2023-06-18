#define CAN_RX  21
#define CAN_TX  22
#define CAN_STATUS_LED 16


void setup(void) {

    // Serial.begin(115200);

    pinMode(CAN_RX, INPUT);
    pinMode(CAN_STATUS_LED, OUTPUT);
}


void loop(void) {

    // Serial.println(digitalRead(CAN_RX));
    digitalWrite(CAN_STATUS_LED, digitalRead(CAN_RX));
}