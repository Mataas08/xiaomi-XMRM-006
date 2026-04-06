#include <Arduino.h>
#include <XiaomiXMRM006.h>

XiaomiXMRM006 remote;

uint32_t lastRead = 0;

void setup() {
  Serial.begin(115200);
  remote.begin("18:bf:1c:96:a0:2d");
}

void loop() {
  remote.update();

  if (millis() - lastRead >= 2000) {
    lastRead = millis();

    int battery = remote.getBatteryLevel();

    if (battery >= 0) {
      Serial.print("Battery: ");
      Serial.print(battery);
      Serial.println("%");
    }
  }
}