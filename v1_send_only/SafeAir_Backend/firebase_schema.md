# SafeAir Firebase Realtime Database Schema

## Root: `/devices/{deviceId}/`

| Path | Type | Writer | Reader | TTL / Notes |
|------|------|--------|--------|-------------|
| `telemetry` | Object | ESP32 | PWA | Overwritten every 3s. Cloud Function archives to `/history`. |
| `heartbeat` | Object | ESP32 | PWA | Overwritten every 15s. |
| `commands/{cmdId}` | Object | PWA | ESP32 | Device deletes after ack. |
| `ack/{pushId}` | Object | ESP32 | PWA | Auto-cleanup after 24h via CF. |
| `logs/{pushId}` | Object | ESP32 | PWA | Auto-cleanup after 7d via CF. |
| `meta/thresholds` | Object | PWA | ESP32 | Device loads on boot / on change. |
| `meta/info` | Object | ESP32 | PWA | deviceName, roomName, fw, mac, model. |
| `alerts/{alertId}` | Object | Cloud Fn | PWA | Created on danger threshold crossing. |

### Telemetry payload (example)
```json
{
  "ts": 123456789,
  "deviceId": "kitchen-1",
  "uptime": 3600,
  "rssi": -52,
  "alarmState": "safe",
  "sensors": {
    "lpg": 14,
    "methane": 22,
    "temp": 27.0,
    "humidity": 45.0,
    "smoke": 8
  }
}
```

### Command payload (example)
```json
{
  "id": "cmd-7f3a",
  "type": "mute",
  "ts": 123456789,
  "payload": {}
}
```

### Ack payload (example)
```json
{
  "ts": 123456790,
  "cmdId": "cmd-7f3a",
  "status": "success",
  "message": "done"
}
```

## Root: `/users/{uid}/` (Phase 2)
```json
{
  "email": "user@example.com",
  "devices": {
    "kitchen-1": true,
    "garage-1": true
  }
}
```
