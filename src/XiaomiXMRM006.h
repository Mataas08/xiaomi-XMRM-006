#ifndef XIAOMI_XMRM006_H
#define XIAOMI_XMRM006_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLESecurity.h>

class XiaomiXMRM006
{
public:
  struct ButtonEvent
  {
    String name;      // e.g. "OK", "UP", "BACK", "UNKNOWN"
    uint32_t rawMask; // e.g. 0x000001
    bool pressed;     // true = PRESS, false = RELEASE
    bool valid;       // true if event is valid
  };

  XiaomiXMRM006();

  // Library initialization
  bool begin(const String &macAddress,
             const char *deviceName = "ESP32-C3-Receiver");

  // Must be called inside loop()
  void update();

  // State
  bool isConnected() const;
  bool isReady() const;

  // Configuration
  void setReconnectInterval(uint32_t ms);
  void setAutoReconnect(bool enable);
  void setBatteryReadInterval(uint32_t ms);

  // Button event reading
  bool available() const;
  ButtonEvent readEvent();

  // Convenient queue-based reads
  String readDecoded();
  uint32_t readRaw();

  // Last event access
  String getLastDecoded() const;
  uint32_t getLastRaw() const;
  bool getLastPressedState() const;

  // Battery API
  int getBatteryLevel();           // Returns battery level or -1 if unavailable
  int getLastBatteryLevel() const; // Returns cached battery level or -1
  bool hasBatteryLevel() const;    // True if battery has been read successfully

private:
  struct ButtonMap
  {
    uint32_t mask;
    const char *name;
  };

  // BLE UUIDs
  static const BLEUUID HID_SERVICE_UUID;
  static const BLEUUID REPORT_CHAR_UUID;
  static const BLEUUID BATTERY_SERVICE_UUID;
  static const BLEUUID BATTERY_LEVEL_CHAR_UUID;

  static XiaomiXMRM006 *s_instance;

  BLEClient *_client;
  BLEAddress *_targetAddress;
  BLERemoteCharacteristic *_reportChar;
  BLERemoteCharacteristic *_batteryChar;

  bool _connected;
  bool _ready;
  bool _charInitialized;
  bool _autoReconnect;

  uint32_t _reconnectIntervalMs;
  uint32_t _lastReconnectAttempt;
  uint32_t _prevMask;

  String _macAddress;
  String _deviceName;

  // Battery state
  int _batteryLevel;
  bool _batteryValid;
  uint32_t _lastBatteryReadMs;
  uint32_t _batteryReadIntervalMs;

  // Event queue
  ButtonEvent _queue[16];
  volatile uint8_t _queueHead;
  volatile uint8_t _queueTail;

  ButtonEvent _lastEvent;

  static void notifyCallback(BLERemoteCharacteristic *pChar,
                             uint8_t *data,
                             size_t length,
                             bool isNotify);

  void decodeButtons(uint8_t *data, size_t length);
  void pushEvent(const ButtonEvent &event);
  bool popEvent(ButtonEvent &event);
  const char *findButtonName(uint32_t mask) const;
  bool connectToRemote();

  bool initRemoteCharacteristics();
  bool initBatteryService();
  bool readBatteryLevelInternal();

  class MyClientCallback : public BLEClientCallbacks
  {
  public:
    void onConnect(BLEClient *pclient) override;
    void onDisconnect(BLEClient *pclient) override;
  };

  MyClientCallback _clientCallback;

  static const ButtonMap _pilotButtons[];
  static const int _buttonCount;
};

#endif