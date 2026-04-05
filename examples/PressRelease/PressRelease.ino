#include <Arduino.h>
#include <XiaomiXMRM006.h>

XiaomiXMRM006 remote;

void setup() {
  Serial.begin(115200);
  remote.begin("18:bf:1c:96:a0:2d");
}

void loop() {
  remote.update();

  while (remote.available()) {
    auto ev = remote.readEvent();

    if (ev.valid) {
      if (ev.pressed) {
        Serial.println("PRESS");
      } else {
        Serial.println("RELEASE");
      }
    }
  }
}