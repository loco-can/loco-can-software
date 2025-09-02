
class ControllerClass {
  public:
    virtual void begin(int port);
};

class SpecialClass : ControllerClass {

};

void ControllerClass::begin(int port) {
  Serial.println(port);
};


ControllerClass controller;


void setup(void) {

  Serial.begin(115200);
  controller.begin(123);
}

void loop(void) {

}