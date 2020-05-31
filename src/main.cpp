#include <Arduino.h>

#define LED_PIN D5

void setup() {
  Serial.begin(115200);

  pinMode(D5, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("blinky");

  delay(1000);

  digitalWrite(LED_PIN, LOW);

  delay(1000);
}
