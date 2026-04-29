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

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== Xiaomi XMRM-006 example ===");

  remote.setReconnectInterval(5000);   // default: 5000 ms
  remote.setAutoReconnect(true);       // default: enabled

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

  // Simple connection check
  if (!remote.isConnected()) {
    // You can add LED or status handling here
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