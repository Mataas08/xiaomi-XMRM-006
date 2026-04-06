#include "XiaomiXMRM006.h"

// ==================== CONSTANTS ====================
const BLEUUID XiaomiXMRM006::HID_SERVICE_UUID((uint16_t)0x1812);
const BLEUUID XiaomiXMRM006::REPORT_CHAR_UUID((uint16_t)0x2A4D);
const BLEUUID XiaomiXMRM006::BATTERY_SERVICE_UUID((uint16_t)0x180F);
const BLEUUID XiaomiXMRM006::BATTERY_LEVEL_CHAR_UUID((uint16_t)0x2A19);

XiaomiXMRM006 *XiaomiXMRM006::s_instance = nullptr;

const XiaomiXMRM006::ButtonMap XiaomiXMRM006::_pilotButtons[] = {
    {0x000001, "OK"},
    {0x000002, "UP"},
    {0x000004, "DOWN"},
    {0x000008, "LEFT"},
    {0x000010, "RIGHT"},
    {0x000020, "POWER"},
    {0x000040, "MIC/VOICE"},
    {0x000080, "VOL_UP"},
    {0x000100, "VOL_DOWN"},
    {0x000200, "MENU/PATCHES"},
    {0x000400, "NETFLIX"},
    {0x002000, "PRIME VIDEO"},
    {0x040000, "HOME"},
    {0x080000, "BACK"}};

const int XiaomiXMRM006::_buttonCount =
    sizeof(XiaomiXMRM006::_pilotButtons) / sizeof(XiaomiXMRM006::_pilotButtons[0]);

// ==================== CONSTRUCTOR ====================
XiaomiXMRM006::XiaomiXMRM006()
    : _client(nullptr),
      _targetAddress(nullptr),
      _reportChar(nullptr),
      _batteryChar(nullptr),
      _connected(false),
      _ready(false),
      _charInitialized(false),
      _autoReconnect(true),
      _reconnectIntervalMs(5000),
      _lastReconnectAttempt(0),
      _prevMask(0),
      _batteryLevel(-1),
      _batteryValid(false),
      _lastBatteryReadMs(0),
      _batteryReadIntervalMs(60000),
      _queueHead(0),
      _queueTail(0)
{
  _lastEvent = {"", 0, false, false};
}

// ==================== CLIENT CALLBACKS ====================
void XiaomiXMRM006::MyClientCallback::onConnect(BLEClient *pclient)
{
  if (XiaomiXMRM006::s_instance)
  {
    XiaomiXMRM006::s_instance->_connected = true;
  }
}

void XiaomiXMRM006::MyClientCallback::onDisconnect(BLEClient *pclient)
{
  if (XiaomiXMRM006::s_instance)
  {
    XiaomiXMRM006::s_instance->_connected = false;
    XiaomiXMRM006::s_instance->_ready = false;
    XiaomiXMRM006::s_instance->_charInitialized = false;
    XiaomiXMRM006::s_instance->_prevMask = 0;
    XiaomiXMRM006::s_instance->_reportChar = nullptr;
    XiaomiXMRM006::s_instance->_batteryChar = nullptr;
    XiaomiXMRM006::s_instance->_batteryValid = false;
    XiaomiXMRM006::s_instance->_batteryLevel = -1;
    XiaomiXMRM006::s_instance->_lastBatteryReadMs = 0;
  }
}

// ==================== NOTIFY CALLBACK ====================
void XiaomiXMRM006::notifyCallback(BLERemoteCharacteristic *pChar,
                                   uint8_t *data,
                                   size_t length,
                                   bool isNotify)
{
  if (XiaomiXMRM006::s_instance)
  {
    XiaomiXMRM006::s_instance->decodeButtons(data, length);
  }
}

// ==================== BEGIN ====================
bool XiaomiXMRM006::begin(const String &macAddress, const char *deviceName)
{
  _macAddress = macAddress;
  _deviceName = deviceName ? String(deviceName) : String("ESP32-C3-Receiver");

  if (_targetAddress)
  {
    delete _targetAddress;
    _targetAddress = nullptr;
  }

  _targetAddress = new BLEAddress(_macAddress.c_str());
  if (!_targetAddress)
  {
    return false;
  }

  s_instance = this;

  BLEDevice::init(_deviceName.c_str());

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  pSecurity->setCapability(ESP_IO_CAP_NONE);

  _client = BLEDevice::createClient();
  if (!_client)
  {
    return false;
  }

  _client->setClientCallbacks(&_clientCallback);

  _connected = false;
  _ready = false;
  _charInitialized = false;
  _reportChar = nullptr;
  _batteryChar = nullptr;
  _prevMask = 0;
  _queueHead = 0;
  _queueTail = 0;
  _lastReconnectAttempt = 0;
  _batteryLevel = -1;
  _batteryValid = false;
  _lastBatteryReadMs = 0;

  return connectToRemote();
}

// ==================== CONNECT ====================
bool XiaomiXMRM006::connectToRemote()
{
  if (!_client || !_targetAddress)
  {
    return false;
  }

  if (_client->isConnected())
  {
    return true;
  }

  return _client->connect(*_targetAddress);
}

// ==================== INTERNAL CHARACTERISTIC INIT ====================
bool XiaomiXMRM006::initRemoteCharacteristics()
{
  if (!_client || !_client->isConnected())
  {
    return false;
  }

  BLERemoteService *pSvc = _client->getService(HID_SERVICE_UUID);
  if (!pSvc)
  {
    return false;
  }

  _reportChar = pSvc->getCharacteristic(REPORT_CHAR_UUID);
  if (!_reportChar)
  {
    return false;
  }

  if (_reportChar->canNotify())
  {
    _reportChar->registerForNotify(notifyCallback);
    _charInitialized = true;
    _ready = true;
    return true;
  }

  return false;
}

// ==================== BATTERY INIT ====================
bool XiaomiXMRM006::initBatteryService()
{
  if (!_client || !_client->isConnected())
  {
    return false;
  }

  BLERemoteService *pBatterySvc = _client->getService(BATTERY_SERVICE_UUID);
  if (!pBatterySvc)
  {
    _batteryChar = nullptr;
    _batteryValid = false;
    return false;
  }

  _batteryChar = pBatterySvc->getCharacteristic(BATTERY_LEVEL_CHAR_UUID);
  if (!_batteryChar)
  {
    _batteryValid = false;
    return false;
  }

  return readBatteryLevelInternal();
}

// ==================== BATTERY READ ====================
bool XiaomiXMRM006::readBatteryLevelInternal()
{
  if (!_batteryChar || !_batteryChar->canRead())
  {
    return false;
  }

  String value = _batteryChar->readValue();
  if (value.length() < 1)
  {
    return false;
  }

  _batteryLevel = (uint8_t)value[0];
  _batteryValid = true;
  _lastBatteryReadMs = millis();
  return true;
}

// ==================== UPDATE ====================
void XiaomiXMRM006::update()
{
  if (!_client)
  {
    return;
  }

  if (_connected)
  {
    if (!_charInitialized)
    {
      initRemoteCharacteristics();
    }

    if (!_batteryChar)
    {
      initBatteryService();
    }
    else if (millis() - _lastBatteryReadMs >= _batteryReadIntervalMs)
    {
      readBatteryLevelInternal();
    }
  }
  else
  {
    _ready = false;

    if (_autoReconnect)
    {
      if (millis() - _lastReconnectAttempt > _reconnectIntervalMs)
      {
        _lastReconnectAttempt = millis();
        if (!_client->isConnected())
        {
          connectToRemote();
        }
      }
    }
  }
}

// ==================== BUTTON DECODING ====================
void XiaomiXMRM006::decodeButtons(uint8_t *data, size_t length)
{
  if (length < 3)
    return;

  uint32_t newMask = ((uint32_t)data[0]) |
                     ((uint32_t)data[1] << 8) |
                     ((uint32_t)data[2] << 16);

  if (newMask == _prevMask)
    return;

  for (int i = 0; i < _buttonCount; i++)
  {
    bool wasPressed = (_prevMask & _pilotButtons[i].mask) != 0;
    bool isPressed = (newMask & _pilotButtons[i].mask) != 0;

    if (!wasPressed && isPressed)
    {
      ButtonEvent ev;
      ev.name = _pilotButtons[i].name;
      ev.rawMask = _pilotButtons[i].mask;
      ev.pressed = true;
      ev.valid = true;
      pushEvent(ev);
      _lastEvent = ev;
    }
    else if (wasPressed && !isPressed)
    {
      ButtonEvent ev;
      ev.name = _pilotButtons[i].name;
      ev.rawMask = _pilotButtons[i].mask;
      ev.pressed = false;
      ev.valid = true;
      pushEvent(ev);
      _lastEvent = ev;
    }
  }

  // Handle unknown button only for transition 0 -> something
  if (newMask != 0 && _prevMask == 0)
  {
    bool found = false;
    for (int i = 0; i < _buttonCount; i++)
    {
      if (newMask & _pilotButtons[i].mask)
      {
        found = true;
        break;
      }
    }

    if (!found)
    {
      ButtonEvent ev;
      ev.name = "UNKNOWN";
      ev.rawMask = newMask;
      ev.pressed = true;
      ev.valid = true;
      pushEvent(ev);
      _lastEvent = ev;
    }
  }

  _prevMask = newMask;
}

// ==================== QUEUE ====================
void XiaomiXMRM006::pushEvent(const ButtonEvent &event)
{
  uint8_t nextHead = (_queueHead + 1) % 16;

  // If queue is full, overwrite the oldest entry
  if (nextHead == _queueTail)
  {
    _queueTail = (_queueTail + 1) % 16;
  }

  _queue[_queueHead] = event;
  _queueHead = nextHead;
}

bool XiaomiXMRM006::popEvent(ButtonEvent &event)
{
  if (_queueHead == _queueTail)
  {
    return false;
  }

  event = _queue[_queueTail];
  _queueTail = (_queueTail + 1) % 16;
  return true;
}

// ==================== PUBLIC API ====================
bool XiaomiXMRM006::isConnected() const
{
  return _connected;
}

bool XiaomiXMRM006::isReady() const
{
  return _ready;
}

void XiaomiXMRM006::setReconnectInterval(uint32_t ms)
{
  _reconnectIntervalMs = ms;
}

void XiaomiXMRM006::setAutoReconnect(bool enable)
{
  _autoReconnect = enable;
}

void XiaomiXMRM006::setBatteryReadInterval(uint32_t ms)
{
  _batteryReadIntervalMs = ms;
}

bool XiaomiXMRM006::available() const
{
  return _queueHead != _queueTail;
}

XiaomiXMRM006::ButtonEvent XiaomiXMRM006::readEvent()
{
  ButtonEvent ev;
  if (popEvent(ev))
  {
    _lastEvent = ev;
    return ev;
  }

  return {"", 0, false, false};
}

String XiaomiXMRM006::readDecoded()
{
  ButtonEvent ev = readEvent();
  if (ev.valid)
  {
    return ev.name;
  }
  return "";
}

uint32_t XiaomiXMRM006::readRaw()
{
  ButtonEvent ev = readEvent();
  if (ev.valid)
  {
    return ev.rawMask;
  }
  return 0;
}

String XiaomiXMRM006::getLastDecoded() const
{
  return _lastEvent.name;
}

uint32_t XiaomiXMRM006::getLastRaw() const
{
  return _lastEvent.rawMask;
}

bool XiaomiXMRM006::getLastPressedState() const
{
  return _lastEvent.pressed;
}

// ==================== BATTERY PUBLIC API ====================
int XiaomiXMRM006::getBatteryLevel()
{
  if (!_connected)
  {
    return -1;
  }

  if (!_batteryChar)
  {
    initBatteryService();
  }

  if (!_batteryChar)
  {
    return -1;
  }

  if (!_batteryValid || (millis() - _lastBatteryReadMs >= _batteryReadIntervalMs))
  {
    readBatteryLevelInternal();
  }

  return _batteryValid ? _batteryLevel : -1;
}

int XiaomiXMRM006::getLastBatteryLevel() const
{
  return _batteryValid ? _batteryLevel : -1;
}

bool XiaomiXMRM006::hasBatteryLevel() const
{
  return _batteryValid;
}

// ==================== HELPERS ====================
const char *XiaomiXMRM006::findButtonName(uint32_t mask) const
{
  for (int i = 0; i < _buttonCount; i++)
  {
    if (_pilotButtons[i].mask == mask)
    {
      return _pilotButtons[i].name;
    }
  }
  return "UNKNOWN";
}