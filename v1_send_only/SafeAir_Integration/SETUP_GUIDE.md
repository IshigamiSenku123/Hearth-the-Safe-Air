# SafeAir Firebase Setup — Step by Step (No Command Line Required)

## Part 1: Create Your Firebase Project (5 minutes)

### Step 1: Go to Firebase Console
Open [https://console.firebase.google.com](https://console.firebase.google.com) in your browser.
Sign in with your Google account.

### Step 2: Create Project
1. Click **"Create a project"**
2. Name it: `your-project-id` (or any name you want)
3. Disable Google Analytics for now (you can add it later)
4. Click **Create project**
5. Wait ~30 seconds for it to finish

### Step 3: Create a Web App
1. On your project dashboard, click the **</>** icon ("Add app")
2. Choose **"Web"**
3. Nickname: `SafeAir PWA`
4. Click **Register app**
5. You will see a block of code like this — **COPY IT**:

```javascript
const firebaseConfig = {
  apiKey: "YOUR_FIREBASE_API_KEY...",
  authDomain: "PASTE_YOUR_AUTH_DOMAIN",
  databaseURL: "PASTE_YOUR_DATABASE_URL",
  projectId: "PASTE_YOUR_PROJECT_ID",
  storageBucket: "YOUR_PROJECT.appspot.com",
  messagingSenderId: "123456789",
  appId: "1:123456789:web:abcdef"
};
```

**Save this somewhere** — you need `apiKey` and `databaseURL`.

### Step 4: Create Realtime Database
1. In the left sidebar, click **Build → Realtime Database**
2. Click **Create Database**
3. Choose **Start in test mode** (this allows reads/writes without auth — fine for MVP)
4. Pick a region close to you (e.g., `us-central1`)
5. Click **Enable**

Your database URL will be: `PASTE_YOUR_DATABASE_URL`

### Step 5: Set Security Rules
1. In Realtime Database, click the **Rules** tab
2. Delete the default rules
3. Copy-paste the contents of `firebase_rules.json` (from the backend zip)
4. Click **Publish**

---

## Part 2: Update Your Firmware (2 minutes)

Open `Config.h` in Arduino IDE and replace these two lines:

```cpp
#define FB_HOST       "PASTE_YOUR_DB_HOST"
#define FB_API_KEY    "PASTE_YOUR_API_KEY_HERE"
```

Then re-flash the ESP32.

---

## Part 3: Update Your PWA (5 minutes)

Open `safeair.html` and do exactly these 3 things:

### 3a. Add Firebase SDK in the `<head>`
Paste this ABOVE your existing `<script>` tag:

```html
<script type="module">
  import { initializeApp } from "https://www.gstatic.com/firebasejs/10.12.0/firebase-app.js";
  import { getDatabase, ref, onValue, set, push, child } from "https://www.gstatic.com/firebasejs/10.12.0/firebase-database.js";

  const firebaseConfig = {
    apiKey: "PASTE_YOUR_API_KEY",
    authDomain: "PASTE_YOUR_AUTH_DOMAIN",
    databaseURL: "PASTE_YOUR_DATABASE_URL",
    projectId: "PASTE_YOUR_PROJECT_ID",
    storageBucket: "YOUR_PROJECT.appspot.com",
    messagingSenderId: "PASTE_YOURS",
    appId: "PASTE_YOURS"
  };

  const app = initializeApp(firebaseConfig);
  const db = getDatabase(app);
  window.safeairDB = db;  // make it available to your existing code
</script>
```

### 3b. Replace simulated data with real data
In your `safeair.html`, wherever you have `state.sensors = {...}` with fake numbers, replace it with:

```javascript
// Subscribe to live data from your device
const deviceId = "kitchen-1";  // same as in Config.h device name
const telemRef = ref(window.safeairDB, `devices/${deviceId}/telemetry`);

onValue(telemRef, (snapshot) => {
  const data = snapshot.val();
  if (!data) return;

  // Map Firebase data to your existing state structure
  state.sensors.lpg.value    = data.sensors?.lpg    || 0;
  state.sensors.gas.value    = data.sensors?.methane || 0;
  state.sensors.smoke.value  = data.sensors?.smoke  || 0;
  state.sensors.temp.value   = data.sensors?.temp   || 0;

  // Update alarm state
  state.overallStatus = data.alarmState || 'safe';

  // Refresh the UI
  renderShell();
});
```

### 3c. Send commands to the device
When the user clicks "Mute" or "Test Alarm" in your PWA:

```javascript
function sendCommand(type, payload = {}) {
  const cmdRef = ref(window.safeairDB, `devices/${deviceId}/commands`);
  const newCmd = push(cmdRef);
  set(newCmd, {
    id: newCmd.key,
    type: type,        // "mute", "test", "arm", "disarm", "pump"
    ts: Date.now(),
    payload: payload
  });
}

// Example usage:
// sendCommand("mute");
// sendCommand("test");
// sendCommand("arm");
```

---

## Part 4: Cloud Functions (Optional for MVP)

Cloud Functions give you:
- Push notifications when danger happens
- Automatic cleanup of old data
- Long-term history archiving

**If you want them:** You need Node.js installed on your computer, then run:
```
npm install -g firebase-tools
firebase login
firebase deploy
```

**If you skip them:** Everything still works. The ESP32 handles alarms locally. The PWA shows live data. You just won't get phone push notifications.

---

## What Should Happen Now

1. **ESP32 boots** → connects to WiFi → writes sensor data to `/devices/kitchen-1/telemetry`
2. **PWA opens** → reads that data in real-time → shows live numbers
3. **User clicks "Mute" in PWA** → writes command to `/devices/kitchen-1/commands`
4. **ESP32 sees command** → mutes buzzer → writes ack to `/devices/kitchen-1/ack`
5. **PWA sees ack** → shows "Command confirmed"

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| "Permission denied" in browser console | RTDB rules are wrong — go to Firebase Console → Database → Rules → paste `firebase_rules.json` |
| ESP32 says "Firebase init failed" | Wrong `FB_API_KEY` in `Config.h` — copy from Firebase Console → Project Settings → General |
| PWA shows old/simulated data | Make sure you replaced the fake `state.sensors` assignment with the `onValue` subscription |
| Device not appearing in database | Check Serial Monitor — ESP32 must connect to WiFi first, then Firebase |
