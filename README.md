# xiaomi-XMRM-006

Arduino library for Xiaomi XMRM-006 BLE remote (ESP32 / ESP32-C3).

Connects to the remote by MAC address and provides:

- decoded button names (`OK`, `POWER`, etc.)
- raw button values (`0x000020`)
- press / release events

---

## Features

- BLE connection by MAC address
- Button decoding
- Raw data access
- Press / release detection
- Event queue
- Connection state
- Ready state
- Auto reconnect

---

## Supported hardware

- ESP32  
- ESP32-C3  

---

## Installation

Copy to:

```text
Documents/Arduino/libraries/xiaomi-XMRM-006
```

Restart Arduino IDE.

---

## Usage

```cpp
#include <XiaomiXMRM006.h>

XiaomiXMRM006 remote;

void setup() {
  remote.begin("18:bf:1c:96:a0:2d");
}

void loop() {
  remote.update();

  while (remote.available()) {
    auto ev = remote.readEvent();
  }
}
```

---

## Required

### begin()

```cpp
remote.begin("MAC_ADDRESS");
```

Start connection.

---

### update()

```cpp
remote.update();
```

Call in every loop.

---

## Status

### isConnected()

```cpp
remote.isConnected();
```

- `true` → connected  
- `false` → not connected  

---

### isReady()

```cpp
remote.isReady();
```

- `true` → ready (working)  
- `false` → not ready  

---

## Reading data

### available()

```cpp
remote.available();
```

- `true` → event available  
- `false` → no data  

---

### readEvent()

```cpp
auto ev = remote.readEvent();
```

Returns:

```cpp
ev.name      // String → "OK", "POWER"
ev.rawMask   // uint32_t → 0x000020
ev.pressed   // bool → true = press, false = release
ev.valid     // bool
```

---

### readDecoded()

```cpp
remote.readDecoded();
```

Returns:
- "OK", "POWER"  
- "" if empty  

---

### readRaw()

```cpp
remote.readRaw();
```

Returns:
- 0x000020  
- 0 if empty  

---

## Last event

```cpp
remote.getLastDecoded();
remote.getLastRaw();
remote.getLastPressedState();
```

Returns data from the **last processed event** (without removing anything from queue).

- `getLastDecoded()` → last button name (`"OK"`, `"POWER"`)
- `getLastRaw()` → last raw value (`0x000020`)
- `getLastPressedState()` → `true` = press, `false` = release

---

## Config

```cpp
remote.setReconnectInterval(5000);
remote.setAutoReconnect(true);
```

- `setReconnectInterval(ms)` → reconnect interval in milliseconds  
- `setAutoReconnect(true/false)` → enable or disable auto reconnect  

Default:
- reconnect enabled  
- interval = 5000 ms  

---

## Buttons

| Name        | Raw       |
|------------|----------|
| OK         | 0x000001 |
| UP         | 0x000002 |
| DOWN       | 0x000004 |
| LEFT       | 0x000008 |
| RIGHT      | 0x000010 |
| POWER      | 0x000020 |
| MIC        | 0x000040 |
| VOL_UP     | 0x000080 |
| VOL_DOWN   | 0x000100 |
| MENU       | 0x000200 |
| NETFLIX    | 0x000400 |
| PRIME      | 0x002000 |
| HOME       | 0x040000 |
| BACK       | 0x080000 |

---

## Notes

- call `update()` in loop  
- use `readEvent()` for full data  
- `readDecoded()` / `readRaw()` remove events from queue  
- remote may need button press to wake up  
