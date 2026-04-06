#include <Arduino.h>

#if !defined(ARDUINO_ARCH_ESP32)
  #error "This library works only on ESP32."
#endif

#ifndef CONFIG_IDF_TARGET_ESP32C3
  #warning "This example is mainly intended for ESP32-C3."
#endif

#include <XiaomiXMRM006.h>

XiaomiXMRM006 remote;

// Enter your remote MAC address
static const char* REMOTE_MAC = "18:bf:1c:96:a0:2d";

bool readyMessageShown = false;

uint32_t lastBatteryLog = 0;
const uint32_t batteryInterval = 5000; // 5 sekund

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== Xiaomi XMRM-006 example ===");

  remote.setReconnectInterval(5000);
  remote.setAutoReconnect(true);
  remote.setBatteryReadInterval(10000);

  bool ok = remote.begin(REMOTE_MAC);

  if (ok) {
    Serial.println("Connection attempt started.");
  } else {
    Serial.println("Failed to initialize library.");
  }

  Serial.println("Press any button on the remote to wake it up.");
}

void loop() {
  remote.update();

  // Ready state info
  if (remote.isReady() && !readyMessageShown) {
    readyMessageShown = true;
    Serial.println(">> Remote READY!");
  }

  if (!remote.isReady() && readyMessageShown) {
    readyMessageShown = false;
    Serial.println(">> Remote NOT ready.");
  }
  
  if (millis() - lastBatteryLog >= batteryInterval) {
    lastBatteryLog = millis();

    int battery = remote.getBatteryLevel();

    Serial.print("[BATTERY] ");

    if (battery >= 0) {
      Serial.print(battery);
      Serial.println("%");
    } else {
      Serial.println("not available");
    }
  }

  // Read events
  while (remote.available()) {
    XiaomiXMRM006::ButtonEvent ev = remote.readEvent();

    if (ev.valid) {
      Serial.print(ev.pressed ? "PRESS:   " : "RELEASE: ");
      Serial.print(ev.name);
      Serial.print(" | RAW: 0x");
      Serial.println(ev.rawMask, HEX);
    }
  }

  delay(20);
}