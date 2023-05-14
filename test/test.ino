#include <Arduino.h>


#include "value.h"
#include "measure_calculate.h"


MEASURE_CALCULATE value_calc;
VALUE value;


void setup(void) {


  Serial.begin(115200);

  value.begin(12);
  value_calc.begin();

}


void loop(void) {

  uint16_t v;
  uint16_t i;

  for (i=0;i<10;i++) {

    v = random(12000);

    value.set(v);

    Serial.print(value.percentage());
    Serial.print(" ");

    value_calc.add_max(value);
  }

  Serial.println();

  Serial.print(" avr ");
  Serial.println(value_calc.percentage());

  delay(500);

  value_calc.reset();

}


void print(VALUE value) {

  Serial.println("stuct MEASURE_VALUE {");
  Serial.print("  reference=");
  Serial.println(value.reference());
  Serial.print("  precission=");
  Serial.println(value.precission());
  Serial.print("  percentage=");
  Serial.println(value.percentage());
  Serial.print("  index=");
  Serial.println(value.index());
  Serial.println("}");

}