# SafeAir Backend — Stage 2

## Architecture

```
ESP32-S3 ──► Firebase RTDB ◄─── React PWA
     │            │
     │            ▼
     │      Cloud Functions (Node.js 20)
     │      ├── onTelemetry  → threshold check → alert
     │      ├── onDangerAlert → FCM push notification
     │      ├── onCommandAck → cleanup old commands
     │      ├── archiveTelemetry → /history (hourly)
     │      └── provisionDevice (HTTP)
     │
     └──► (Phase 2) NestJS/FastAPI backend
```

## Phase 1 (MVP) — what you have now

- **No custom backend server.** ESP32 talks directly to Firebase RTDB.
- **Public RTDB rules** — anyone with the `deviceId` can read/write.
- **Anonymous Firebase auth** on ESP32 (MVP only).
- **Cloud Functions** handle alerting, notifications, and data cleanup.

## Phase 2 (Commercial) — migration path

1. Add Firebase Auth (email/password or Google OAuth) to the PWA.
2. Replace RTDB rules with `auth.uid` based ownership (`/users/{uid}/devices`).
3. Add a **NestJS or FastAPI** backend that:
   - Manages user accounts & organizations
   - Owns device registrations
   - Generates PDF reports from `/history`
   - Coordinates OTA updates
   - Exposes REST APIs for third-party integrations
4. ESP32 continues talking directly to Firebase for real-time telemetry (low latency).
   The backend subscribes to RTDB changes or receives Cloud Function webhooks.

## Quick Deploy

### 1. Install Firebase CLI
```bash
npm install -g firebase-tools
firebase login
```

### 2. Create project
```bash
firebase projects:create your-project-id
firebase use your-project-id
```

### 3. Deploy rules + functions
```bash
cd SafeAir_Backend
firebase deploy
```

### 4. Update ESP32 firmware
Edit `Config.h`:
```cpp
#define FB_HOST  "PASTE_YOUR_DB_HOST"
#define FB_API_KEY  "YOUR_WEB_API_KEY"
```
Get the Web API Key from Firebase Console → Project Settings → General.

### 5. PWA connection
The existing `safeair.html` frontend should read from:
```
PASTE_YOUR_DATABASE_URL/devices/{deviceId}/telemetry.json
```
and write commands to:
```
/devices/{deviceId}/commands/{cmdId}.json
```

## RTDB Paths Reference

| Path | Purpose |
|------|---------|
| `/devices/{id}/telemetry` | Live sensor data (overwritten) |
| `/devices/{id}/heartbeat` | Device health (overwritten) |
| `/devices/{id}/commands/{id}` | PWA → device commands |
| `/devices/{id}/ack/{id}` | Device → PWA command responses |
| `/devices/{id}/alerts/{id}` | Danger events (Cloud Function writes) |
| `/devices/{id}/logs/{id}` | Event log entries |
| `/devices/{id}/meta/thresholds` | Alarm thresholds |
| `/devices/{id}/meta/fcmTokens/{token}` | Push notification tokens |
| `/history/{id}/{YYYY-MM-DD}/{ts}` | Archived telemetry (hourly CF) |

## FCM Push Notifications

1. In Firebase Console → Cloud Messaging, get your server key.
2. The PWA must request notification permission and store the token:
   ```js
   const token = await getToken(messaging, { vapidKey: 'YOUR_VAPID_KEY' });
   // Write to /devices/{id}/meta/fcmTokens/{token}
   ```
3. The `onDangerAlert` Cloud Function automatically sends push notifications
to all registered tokens when a danger alert is created.

## Security Warning (MVP)

The current rules are **public** (`"true"`). This is fine for MVP with physical
device access control, but **must be hardened before commercial launch**.
Phase 2 rules will require Firebase Authentication and validate device ownership.
