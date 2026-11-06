# Turning Anything Into a Smart Device
*NOTE: THE FOLLOWING SECTION IS USING THE PROJECT'S CORE HARDWARE AND MOTIVATION AS REFERENCE EXAMPLE ON HOW TO MAKE YOUR STUFF SMART*
SafeAir started as a smart stove guard, but the parts that make it smart are not stove-specific. Wi-Fi setup, cloud sync, and two-way control can be reused for any device that has sensors or things it can switch on and off.

This guide shows how the system is put together, which files to change for a new device, and how to add a feature from start to finish.

---

## What the firmware needs

Install these before compiling. The full list, with board settings and Firebase steps, is in [SETUP.md](SETUP.md).

| Library | Required | Purpose |
|---|---|---|
| Firebase-ESP-Client (mobizt) | Yes | Cloud stream and writes (`FirebaseManager`) |
| Adafruit GFX + Adafruit SSD1306 | Yes | OLED display |
| DHT sensor library + Adafruit Unified Sensor | Yes | DHT11 temperature and humidity |
| ArduinoJson | Likely | Building JSON in `JSONManager` (confirm) |
| `Wire`, `WiFi`, `WebServer`, `Preferences` | Built in | I²C, Wi-Fi, setup page, storage |

---

## The core idea

Three files make a device "smart." Everything else is hardware-specific.

| File | Job | Reusable as-is? |
|---|---|---|
| `WiFiManager.cpp/.h` | Hotspot setup, saving credentials, reconnecting | Yes. Change only the hotspot name prefix |
| `DeviceManager.cpp/.h` | Sends telemetry and heartbeats, and maps every command to an action | No. This is where your device's actions are defined |
| `main.ino` | Starts the tasks and wires everything together | Mostly. Add or remove tasks for your hardware |

Two supporting files also change for each new device:

- `Config.h`: pin numbers, timing, and thresholds.
- Your sensor and actuator drivers: one small module per piece of hardware.

Everything else (`FirebaseManager`, `JSONManager`, `Logger`, `StorageManager`) can be reused as-is.

All of these live together in one folder, `firmware_final/main/`. Arduino IDE builds every `.h` and `.cpp` file in that folder, so a new driver goes there too.

---

## Control your device from anywhere, without websockets

Most DIY "smart" projects put a web server on the device and try to reach it from your phone. That only works on the same Wi-Fi network, and it usually needs port forwarding or a static IP to work outside home.

SafeAir takes a different route. The device and the phone never talk to each other directly. They both talk to Firebase Realtime Database, over normal HTTPS, on the internet.

```
 Phone on mobile data  ──HTTPS──▶  Firebase RTDB  ◀──HTTPS──  Device on home Wi-Fi
 (any network, anywhere)            (the relay)              (any network, no open ports)
```

What this gives you:

- **No shared network needed.** Your phone can be on 4G in another city. The device can be behind a router, a hotel firewall, or a phone hotspot.
- **No open ports or port forwarding.** The device only makes outbound connections, so there is nothing for an attacker to reach from the internet.
- **No IP address to track.** The device can change IP addresses freely; the database address never changes.
- **Real-time both ways.** The device holds a stream open to `devices/{id}/commands`, so a command arrives as soon as it's written, with no polling.
- **Confirmed actions.** The device writes an acknowledgement back, so the phone knows the action really happened.

The two classes that make this work:

- **`WiFiManager`** gets the device onto your network, even when the network is unknown at build time.
- **`FirebaseManager`** keeps the telemetry, heartbeat, command stream, and acknowledgements flowing.

Together they are a reusable smart-device kit. You do not need to write a web server, a socket layer, or a network discovery system.

### Trade-offs to know about

- **Internet required.** If the house loses internet, the phone can't reach the device. The device still runs its local safety logic.
- **Latency depends on the cloud.** Commands usually arrive in well under a second on a good connection, but they're not instant like a local button.
- **Your data sits with a third party.** Telemetry passes through Firebase. Use the security rules and authentication before storing anything sensitive.

---

## How a command travels

This is the path a button press takes from the phone to the hardware and back:

```
 Phone (PWA)                 Firebase RTDB                  ESP32
 ───────────                 ─────────────                  ─────
 tap "Test alarm"  ──push──▶ devices/{id}/commands/{cmd}
                                     │
                                     └──stream──▶ FirebaseManager
                                                       │
                                                       ▼
                                              DeviceManager::onCommand()
                                                       │
                                                  does the action
                                                       │
 toast "Command      ◀──────── devices/{id}/ack ◀──────┘
 confirmed"                                       (success or failure)
```

Two things make this reliable:

1. **The device acts, then acknowledges.** The phone shows "confirmed" only after the hardware has done the work, so the app never claims something happened when it didn't.
2. **Every command has an ID.** The acknowledgement carries the same ID, so the app knows which button press it is answering.

---

## Adding a feature: a fan, step by step

This example adds a relay-controlled fan that can be turned on and off from the app. All the files go in `firmware_final/main/`.

### 1. Define the pin (`Config.h`)

```cpp
#define PIN_FAN    16   // Relay driver for the fan
```

### 2. Write a small driver (`FanManager.h`)

Keep hardware code in its own module so the rest of the firmware never touches pins directly.

```cpp
// firmware_final/main/FanManager.h
#pragma once
#include <Arduino.h>
#include "Config.h"

class FanManager {
public:
  void begin()        { pinMode(PIN_FAN, OUTPUT); set(false); }
  void set(bool on)   { digitalWrite(PIN_FAN, on ? HIGH : LOW); _on = on; }
  bool isOn() const   { return _on; }
private:
  bool _on = false;
};

extern FanManager Fan;
```

Then create `FanManager.cpp` in the same folder with `FanManager Fan;` on its own line, so the global object exists.

### 3. Register the command (`DeviceManager.cpp`, inside `onCommand`)

```cpp
} else if (strcmp(cmd.type, "fan_on") == 0) {
  Fan.set(true);
} else if (strcmp(cmd.type, "fan_off") == 0) {
  Fan.set(false);
```

The acknowledgement is already sent at the end of `onCommand`, so you don't need to add one.

### 4. Report its state (`DeviceManager.cpp`, in telemetry)

Add the fan's state to the telemetry JSON so the app can show it:

```cpp
// in JSONManager::buildTelemetry, add:
// "fan": Fan.isOn()
```

### 5. Add the button (app)

```javascript
document.querySelector('[data-action="fan:on"]')
  .addEventListener('click', () => sendDeviceCommand('fan_on'));
```

That's the whole feature: one pin, one driver, two lines of command handling, one button.

---

## Rules for safe smart devices

These matter more than any feature:

- **Local safety never depends on the cloud.** The alarm and suppression logic run on the device. If Wi-Fi is down, the device still protects the kitchen, the room, or the house.
- **Failsafe defaults.** Every output should start in its safe state at boot: fan off, valve closed, heater off. Set this in `begin()`, not after the first command.
- **Don't trust the phone.** Check values on the device before acting. A command that says `pump_arm` should still be refused if the device is in a state where arming is unsafe.
- **Limit what can be sent.** Only the command names in `onCommand` do anything. An unknown command is rejected and reported, not guessed at.
- **Keep secrets out of the device code.** Keys and hosts belong in `Config.h` or a git-ignored `secrets.h`, never in a file you publish.

---

## Ideas for other devices

The same three-file core works for many other projects:

| Device | Sensors | Commands from the app |
|---|---|---|
| Garage gas and CO monitor | MQ-7 (CO), MQ-2 | Vent fan on/off, alarm test |
| Fridge temperature alarm | DS18B20 | Silence alarm, set threshold |
| Smart plant watering | Soil moisture | Water now, pump on/off |
| Door or window monitor | Reed switch | Arm/disarm, lock state |
| Server room climate | DHT22, PIR | Fan speed, alarm mute |
| Greenhouse controller | Temp, humidity, light | Heater, vent, lights |

---

## Checklist for a new device

- [ ] Copy `WiFiManager` unchanged, and change `SOFTAP_SSID_PREFIX` to your device name.
- [ ] Set your pins and thresholds in `Config.h`.
- [ ] Write one driver module per piece of hardware, in `firmware_final/main/`.
- [ ] Set every output to its safe state in `begin()`.
- [ ] Add each action as a branch in `DeviceManager::onCommand`.
- [ ] Add the state of each output to the telemetry JSON.
- [ ] Add the matching buttons to the app.
- [ ] Test each command, then unplug Wi-Fi and confirm the local behaviour still works.
