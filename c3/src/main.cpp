#include <Arduino.h>

void setup() {
  // put your setup code here, to run once:
  pinMode(RGB_BUILTIN, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(RGB_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(RGB_BUILTIN, LOW);
  delay(1000);
}

