#include <Arduino.h>
#include <XiaomiXMRM006.h>

#define LED_PIN 8

XiaomiXMRM006 remote;
bool ledState = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  remote.begin("18:bf:1c:96:a0:2d");
}

void loop() {
  remote.update();

  while (remote.available()) {
    auto ev = remote.readEvent();

    if (ev.valid && ev.pressed && ev.rawMask == 0x000020) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  }
}