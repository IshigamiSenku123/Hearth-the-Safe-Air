# SafeAir / Hearth — Setup Guide

This guide takes you from a blank computer to a running SafeAir device. It covers Arduino IDE on **Windows** and **Ubuntu**, the libraries the firmware needs, board settings, Firebase, flashing, and troubleshooting.

Sections 1–3 and 6–10 are the same on both systems. Section 4 is the Windows path and section 5 is the Ubuntu path.

---

## 1. What you need

| Item | Notes |
|---|---|
| ESP32-S3-DevKitC-1 (N16R8) | Main controller |
| USB-C cable with data lines | Some cables only charge. If nothing appears when you plug in, swap the cable |
| MQ-2, MQ-4, DHT11 sensors | Gas and temperature inputs |
| 0.96" OLED (SSD1306, I²C, 128×64) | Local display |
| 3 buttons, 4 LEDs, buzzer, MOSFET + pump | Local controls and outputs |
| 3S Li-ion battery pack (11.1 V nominal) with protection board (BMS) | Backup power. Required by our design; choose your own pack if your build differs |
| Buck converter(s) (12.6 V → 5 V, 5 V → 3.3 V) | Power the pump, buzzer and relay from the battery, and the ESP32 from 5 V. Size them for the pump's peak current |
| Battery charger or supply for the 3S pack | Keeps the battery charged from mains |
| Fuse or polyfuse on the battery line | Protects wiring |
| Flyback diode across the pump or relay coil | Protects the MOSFET from voltage spikes |
| Pull-up resistor (about 10 kΩ) on the DHT11 data line | Needed for a reliable DHT11 reading. Often included on breakout boards |
| Wiring, terminal blocks, enclosure | Keep mains and low-voltage wiring apart |
| A 2.4 GHz Wi-Fi network | ESP32 does not join 5 GHz-only networks |
| A computer | Windows 10/11 or Ubuntu 22.04/24.04 |

> ⚠️ Battery and mains work can be dangerous. Follow your local electrical rules, and never leave a lithium pack charging unattended.

---

## 2. Software to install

| Tool | Why | Where |
|---|---|---|
| **Arduino IDE 2.x** | Compiles and uploads the firmware | arduino.cc/en/software |
| **ESP32 board package** (Espressif) | Adds ESP32-S3 support to Arduino IDE | Installed from inside Arduino IDE (section 4.3 or 5.4) |
| **Node.js 18+** and **Firebase CLI** | Deploys the Cloud Functions (optional) | nodejs.org, then `npm install -g firebase-tools` |
| **A modern browser** | Firebase Console and the app | Chrome, Edge or Firefox |

---

## 3. Libraries the firmware uses

Install these from **Arduino IDE → Tools → Manage Libraries** (the menu is the same on Windows and Ubuntu). Search for the name, click **Install**, and accept any dependencies the IDE offers.

### Confirmed from the firmware's `#include` lines

| Library to install | Author | Header it provides | Used by |
|---|---|---|---|
| **Adafruit GFX Library** | Adafruit | `Adafruit_GFX.h` | Display |
| **Adafruit SSD1306** | Adafruit | `Adafruit_SSD1306.h` | Display (OLED) |
| **DHT sensor library** | Adafruit | `DHT.h` | Temperature and humidity (DHT11) |
| **Adafruit Unified Sensor** | Adafruit | Dependency of the DHT library | Sensors |

Adafruit GFX will also ask to install **Adafruit BusIO**. Accept it.

### Required by the cloud code (from the Firebase calls in the firmware)

| Library to install | Author | Header it provides | Used by |
|---|---|---|---|
| **Firebase-ESP-Client** | mobizt | `Firebase_ESP_Client.h` | `FirebaseManager` (stream and writes) |

### Likely required (confirm against your `JSONManager`)

| Library to install | Author | Header it provides | Used by |
|---|---|---|---|
| **ArduinoJson** | Benoit Blanchon | `ArduinoJson.h` | `JSONManager` (telemetry, heartbeat, ack) |

### Built into the ESP32 core (nothing to install)

| Header | Used by |
|---|---|
| `Arduino.h` | Everything (core Arduino functions) |
| `Wire.h` | I²C bus for the OLED |
| `WiFi.h` | Station and access-point modes (`WiFiManager`) |
| `WebServer.h` | Setup page at `192.168.4.1` (`WiFiManager`) |
| `Preferences.h` | Saving Wi-Fi credentials and thresholds (`StorageManager`, `WiFiManager`) |
| `ESP.h` | Restart, chip ID for the hotspot name |
| FreeRTOS (via the core) | The five firmware tasks in `main` |

> Tip: note the version number of each library that works for you (shown in Manage Libraries) and add it to your own copy of this table. Library updates occasionally rename functions.

---

## 4. Windows setup

### 4.1 Install Arduino IDE

1. Download the **Windows** installer (Windows 64-bit) from arduino.cc/en/software.
2. Run it and accept the defaults. Allow it through Windows Defender if asked.
3. Open Arduino IDE once so it creates its folders.

### 4.2 USB driver

The ESP32-S3-DevKitC-1 has two USB-C ports. Use the one marked **USB** for the first flash. It uses the chip's built-in USB, so it normally needs no driver.

If the board doesn't appear as a COM port, use the other port labelled **UART** instead. That port uses a USB-to-serial chip (CH343 or CP2102), so install the matching driver from the chip maker's website, then reconnect.

To check, open **Device Manager → Ports (COM & LPT)**. A new entry such as `USB Serial Device (COM5)` or `Silicon Labs CP210x (COM3)` means the board is connected. Note the COM number.

### 4.3 Install the ESP32 board package

1. In Arduino IDE, open **Tools → Board → Boards Manager**.
2. Search for `esp32`.
3. Install **esp32 by Espressif Systems**. The download is large, so allow a few minutes.

### 4.4 Libraries

Open **Tools → Manage Libraries** and install everything in section 3 that is marked as required or confirmed.

### 4.5 Board settings

Under **Tools**, set:

| Menu | Setting |
|---|---|
| Board | ESP32 Arduino → **ESP32S3 Dev Module** |
| USB CDC On Boot | **Enabled** |
| CPU Frequency | 240MHz (default) |
| Flash Size | **16MB (128Mb)** |
| PSRAM | **OPI PSRAM** |
| Partition Scheme | **Huge APP (3MB No OTA/1MB SPIFFS)** (or a 16MB option with at least 3MB for the app) |
| Upload Speed | **921600** |
| Port | The COM port from section 4.2 |

If the Partition Scheme option is missing, choose the closest option that lists about 3MB for the app.

### 4.6 Open the project

Arduino IDE requires the sketch folder to have the same name as the main `.ino` file. The project keeps `main.ino` in a folder called `main` for that reason:

```
firmware_final/
└── main/
    ├── main.ino
    ├── Config.h
    └── ...          (all manager .h and .cpp files)
```

1. Copy the project folder, for example `C:\Users\<you>\Documents\Hearth\`.
2. Open `firmware_final\main\main.ino` from **File → Open**. The IDE opens the whole `main` folder with it.
3. Keep every `.h` and `.cpp` file in the `main` folder. The IDE builds all of them automatically, so don't move them up a level.

If the IDE asks you to move the sketch to a new folder, the folder name and the main file name don't match. Check that the file is `main.ino` and that it sits inside a folder named `main`.

### 4.7 Edit the config

Open `Config.h` and set your values (see section 6). Keep the real key in your local copy only.

### 4.8 Compile and upload

1. Click **Verify** (checkmark) to compile. Fix any missing-library errors by installing the library it names.
2. Click **Upload** (arrow). If it says "Connecting…" and stalls, see the BOOT button method in the troubleshooting section.
3. When the upload finishes, click **Serial Monitor** and set the baud rate to **115200**.

---

## 5. Ubuntu setup

### 5.1 Install Arduino IDE

Use the **Linux 64-bit** download from arduino.cc/en/software. The tarball gives the most reliable USB access:

```bash
# Option A: the official tarball
cd ~/Downloads
tar -xf arduino-ide_*_Linux_64bit.tar.gz
cd arduino-ide_*_Linux_64bit
./arduino-ide
```

Snap works too (`sudo snap install arduino`), but Snap confinement can block serial ports, so prefer the tarball if the board doesn't show up.

### 5.2 Serial permissions

Your user must be able to open the serial port. Run this once, then log out and back in:

```bash
sudo usermod -a -G dialout $USER
```

Check it with `groups`. You should see `dialout` in the list.

### 5.3 Check the board is detected

Plug the board in and run:

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

- `/dev/ttyACM0` means the **USB** port (native USB). Use this one for the first flash.
- `/dev/ttyUSB0` means the **UART** port (USB-to-serial chip).

Ubuntu usually includes the drivers for both chips, so you normally don't need to install anything.

### 5.4 Install the ESP32 board package

1. In Arduino IDE, open **Tools → Board → Boards Manager**.
2. Search for `esp32` and install **esp32 by Espressif Systems**.

### 5.5 Libraries

Open **Tools → Manage Libraries** and install everything in section 3 that is marked as required or confirmed.

### 5.6 Board settings

Under **Tools**, set the same values as section 4.5. For **Port**, choose `/dev/ttyACM0` (or `/dev/ttyUSB0` if you used the UART port).

### 5.7 Open the project

Same rule as section 4.6: `main.ino` lives in a folder called `main`. The project can sit anywhere, for example `~/Hearth/firmware_final/main/`.

```
~/Hearth/firmware_final/main/
├── main.ino
├── Config.h
└── ...              (all manager .h and .cpp files)
```

1. Open `main.ino` from **File → Open** in Arduino IDE. The IDE opens the whole `main` folder with it.
2. Keep every `.h` and `.cpp` file in the `main` folder, so the IDE can build them all.

### 5.8 Compile and upload

Same as section 4.8. Use **Ctrl+R** to verify, **Ctrl+U** to upload, and **Ctrl+Shift+M** for the Serial Monitor.

---

## 6. Firmware configuration

Edit `Config.h`:

```cpp
#define FB_HOST     "your-project-default-rtdb.firebaseio.com"   // no https://, no trailing slash
#define FB_API_KEY  "YOUR_FIREBASE_API_KEY"                      // rotate if it was ever shared
```

Set the device name the firmware uses (for example `kitchen-1`). The app's `DEVICE_ID` must match it.

**Keep real values out of Git.** Either keep them in a `secrets.h` file that is listed in `.gitignore`, or leave placeholders in the committed copy and edit them locally only.

---

## 7. Firebase project

These steps are the same on both systems because they happen in the browser.

1. **Create the project.** Go to the Firebase Console, click **Add project**, and choose a name (for example `your-project-id`).
2. **Enable Realtime Database.** Build → Realtime Database → **Create database**. Pick your region. You can start in locked mode because the rules are set in step 5.
3. **Copy the database URL.** It looks like `https://<project>-default-rtdb.firebaseio.com`. Use the host part only for `FB_HOST`.
4. **Register a web app** for the app. Project settings → General → **Add app** → Web. Copy the `firebaseConfig` object.
5. **Set the rules.** The rules in this repository are open test rules, published only so the project can be demonstrated:
   ```json
   {
     "rules": {
       ".read": true,
       ".write": true
     }
   }
   ```
   Anyone with the database URL can read and write data under these rules. Write your own rules before storing anything real: start from locked rules, allow only the paths your device and app need, and add authentication and App Check.
6. **Cloud Functions (optional).** These need the Blaze pay-as-you-go plan. Light use usually stays within the free allowance.
   ```bash
   cd backend/functions
   npm install
   firebase login
   firebase deploy --only functions
   ```

---

## 8. Flashing and first run

1. Compile and upload from section 4.8 (Windows) or 5.8 (Ubuntu).
2. Open the Serial Monitor at **115200** baud.
3. On first boot, the device creates a hotspot named `SafeAir-<device-id>`.
4. On your phone or laptop, connect to that hotspot and open `192.168.4.1`.
5. Enter your home Wi-Fi name and password, then submit.
6. The device reboots and joins your network. The Serial Monitor should show a Wi-Fi connected message, then Firebase ready.
7. Open the app. The device should appear online, and telemetry should start within about three seconds.

---

## 9. App configuration

1. Open the app source and paste your `firebaseConfig` into `FIREBASE_CONFIG`, or into the git-ignored config file if you use the split layout.
2. Set `DEVICE_ID` to match the firmware.
3. Host the app on a static host: Firebase Hosting, GitHub Pages, or Netlify.
4. Open the URL on your phone and add it to your home screen.

---

## 10. Troubleshooting

### Upload problems

| Symptom | Fix |
|---|---|
| "Failed to connect to ESP32-S3" or stuck at "Connecting…" | Hold the **BOOT** button, tap **RESET**, release **BOOT**, then upload again. |
| Port missing (Windows) | Try the other USB-C port. Install the CH343 or CP2102 driver for the UART port. |
| Port missing (Ubuntu) | Check `groups` includes `dialout`. Log out and back in. |
| Port appears, then disappears (Ubuntu) | ModemManager is grabbing the device. Run `sudo systemctl stop ModemManager`, or remove it with `sudo apt remove modemmanager`. |
| "Permission denied" on `/dev/ttyACM0` (Ubuntu) | Same as above: add your user to `dialout`. |
| "Library not found" | Install the library named in the error from section 3. |
| Compile error about a missing `.h` | Keep every `.h` and `.cpp` in the `main` folder, next to `main.ino`. |
| IDE asks to move the sketch | The folder name and main file name don't match. The main file must be `main.ino` inside a folder named `main`. |

### Runtime problems

| Symptom | Likely cause | Fix |
|---|---|---|
| Hotspot never appears | Setup mode not triggered | Erase saved credentials and reboot |
| Saved Wi-Fi but the device never connects | Wrong password, or a 5 GHz-only network | Use a 2.4 GHz network |
| Firebase init fails | `FB_HOST` includes `https://` or a trailing slash | Use the bare host only |
| Connected, but no telemetry | Database rules block writes, or `DEVICE_ID` doesn't match | Check the rules and the device name |
| Commands ignored | Stream listener not running | Look for "Command received" in the Serial Monitor |
| OLED blank | Wrong I²C address or wiring | Confirm the address is `0x3C` and SDA/SCL are on GPIO 8 and 9 |
| Sensor readings never change | Wiring, or the sensor is still warming up | MQ sensors need several minutes to warm up before readings settle |
| Pump or buzzer doesn't respond on battery power | Buck converter undersized, or the pack's protection board has tripped | Check the converter's rating against the pump's peak current, and reset the BMS by reconnecting the pack |
