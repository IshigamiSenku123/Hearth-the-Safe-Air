# SafeAir Firmware (ESP32)

This is the firmware that runs on the ESP32 microcontroller.

## What it does
- Reads air quality sensors
- Controls pump and alarm based on three-level thresholds (SAFE/WARNING/DANGER)
 Levels themselves can be edited into any range, i,e SAFE, WARNING DANGER threshold Ranges can be edited both in App and Firmware
- Publishes telemetry to Firebase RTDB
- Listens for commands from the web app (mute, test, arm/disarm, pump control, and much more)
- Handles WiFi provisioning via built-in hotspot

## Partner component
This firmware requires the **web app** to send commands. See:
- `../v4_modular_bridge/` (recommended, once added)
- `../v3_pwa_monolith/` (current working PWA)

## Setup
See inline comments in `main.ino` and manager files for configuration.
You will need to fill in your own Firebase credentials in `Config.h` (see `secrets.example.h` for the template).
