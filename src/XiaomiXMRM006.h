#ifndef XIAOMI_XMRM006_H
#define XIAOMI_XMRM006_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLESecurity.h>

class XiaomiXMRM006 {
public:
  struct ButtonEvent {
    String name;       // np. "OK", "UP", "BACK", "UNKNOWN"
    uint32_t rawMask;  // np. 0x000001
    bool pressed;      // true = PRESS, false = RELEASE
    bool valid;        // czy event poprawny
  };

  XiaomiXMRM006();

  // Inicjalizacja biblioteki
  bool begin(const String& macAddress,
             const char* deviceName = "ESP32-C3-Receiver");

  // Wywoływać w loop()
  void update();

  // Stany
  bool isConnected() const;
  bool isReady() const;

  // Konfiguracja
  void setReconnectInterval(uint32_t ms);
  void setAutoReconnect(bool enable);

  // Odczyt zdarzeń
  bool available() const;
  ButtonEvent readEvent();

  // Wygodne odczyty ostatniego eventu
  String readDecoded();     // zwraca nazwę przycisku z kolejki
  uint32_t readRaw();       // zwraca raw mask z kolejki

  String getLastDecoded() const;
  uint32_t getLastRaw() const;
  bool getLastPressedState() const;

private:
  struct ButtonMap {
    uint32_t mask;
    const char* name;
  };

  static const BLEUUID HID_SERVICE_UUID;
  static const BLEUUID REPORT_CHAR_UUID;

  static XiaomiXMRM006* s_instance;

  BLEClient* _client;
  BLEAddress* _targetAddress;

  bool _connected;
  bool _ready;
  bool _charInitialized;
  bool _autoReconnect;

  uint32_t _reconnectIntervalMs;
  uint32_t _lastReconnectAttempt;
  uint32_t _prevMask;

  String _macAddress;
  String _deviceName;

  ButtonEvent _queue[16];
  volatile uint8_t _queueHead;
  volatile uint8_t _queueTail;

  ButtonEvent _lastEvent;

  static void notifyCallback(BLERemoteCharacteristic* pChar,
                             uint8_t* data,
                             size_t length,
                             bool isNotify);

  void decodeButtons(uint8_t* data, size_t length);
  void pushEvent(const ButtonEvent& event);
  bool popEvent(ButtonEvent& event);
  const char* findButtonName(uint32_t mask) const;
  bool connectToRemote();

  class MyClientCallback : public BLEClientCallbacks {
  public:
    void onConnect(BLEClient* pclient) override;
    void onDisconnect(BLEClient* pclient) override;
  };

  MyClientCallback _clientCallback;

  static const ButtonMap _pilotButtons[];
  static const int _buttonCount;
};

#endif