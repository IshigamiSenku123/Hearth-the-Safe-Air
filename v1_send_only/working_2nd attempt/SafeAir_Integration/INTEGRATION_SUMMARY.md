# SafeAir Integration Summary

## What connects to what

```
┌─────────────┐      WiFi       ┌─────────────────┐      HTTPS      ┌──────────────┐
│  ESP32-S3   │ ──────────────► │  Firebase RTDB  │ ◄────────────── │  PWA (HTML)  │
│  (firmware) │  telemetry      │  (cloud)        │  read/write     │  (browser)   │
│             │ ◄────────────── │                 │                 │              │
│             │  commands       │                 │                 │              │
└─────────────┘                 └─────────────────┘                 └──────────────┘
                                        │
                                        ▼
                              ┌─────────────────┐
                              │ Cloud Functions │ (optional for MVP)
                              │ - alerts        │
                              │ - push notifs   │
                              │ - history       │
                              └─────────────────┘
```

## Data flow

### 1. ESP32 → Firebase (every 3 seconds)
```
/devices/kitchen-1/telemetry
{
  "ts": 123456789,
  "sensors": { "lpg": 14, "methane": 22, "temp": 27.0, "humidity": 45, "smoke": 8 },
  "alarmState": "safe",
  "rssi": -52,
  "uptime": 3600
}
```

### 2. PWA reads that data (real-time)
The `pwa_firebase_bridge.js` uses `onValue()` to automatically update
`state.sensors` whenever new telemetry arrives.

### 3. PWA → ESP32 (on user action)
```
/devices/kitchen-1/commands/{autoId}
{
  "id": "-Nabc123",
  "type": "mute",
  "ts": 123456790,
  "payload": {}
}
```

### 4. ESP32 executes command → sends ack
```
/devices/kitchen-1/ack/{autoId}
{
  "cmdId": "-Nabc123",
  "status": "success",
  "message": "done",
  "ts": 123456791
}
```

## File checklist

| File | Where it goes | What it does |
|------|---------------|--------------|
| `pwa_firebase_bridge.js` | Same folder as `safeair.html` | Connects PWA to Firebase |
| `safeair_pwa_patch.html` | Reference only | Shows exactly what to change |
| `firmware_command_patch.cpp` | Reference only | Shows what to add to `.ino` |
| `SETUP_GUIDE.md` | Read it first | Step-by-step Firebase setup |

## Quick test

1. Flash ESP32 with updated `Config.h` (FB_HOST + FB_API_KEY)
2. Open Serial Monitor — you should see "[Firebase] Firebase ready"
3. Open Firebase Console → Database — you should see `/devices/kitchen-1/telemetry`
4. Open `safeair.html` in browser — live numbers should appear
5. Click "Mute" in PWA → check Serial Monitor for "Command received"
