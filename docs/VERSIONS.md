# SafeAir / Hearth — Version History

This is the full record of each stage of the project. For the short version, see the README. Each stage is a complete, self-contained snapshot. They are not merges of each other.

## Terms used throughout

- **Backend**: the server-side infrastructure (Firebase rules, the database layout, and Cloud Functions).
- **Back-Functionality**: the app's ability to register user actions (mute, test, arm, disarm, pump) and have the device carry them out and confirm them.

---

## At a glance

| Stage | Folder | Name | What it adds | Back-Functionality | Status |
|---|---|---|---|---|---|
| V1 | `v1_send_only/` | Send-only pipeline | Device → cloud telemetry and the backend skeleton | No | Reference and backend template |
| V2 | `v2_back_functionality/` | Bidirectional web app | Web app actions run on the ESP32 | Yes | Archived |
| V3 | `v3_pwa_monolith/` | PWA, merged | Installable PWA with Back-Functionality, bridge merged into the HTML | Yes | Archived |
| V4 | `v4_modular_bridge/` | PWA, modular | Bridge separated into `pwa_firebase_bridge.js` | Yes | Recommended app. Folder not yet added (see note) |
| Firmware | `firmware_final/main/` | Final firmware | Final ESP32 firmware (`main.ino` and its managers) | Yes (bidirectional schema from V2) | Recommended firmware |

> **Note on V4:** the modular bridge folder isn't in the project yet. Until it's added, assemble V4 from the bridge file in `v2_back_functionality/` and the app HTML in `v3_pwa_monolith/`.

### Earlier working names (for reference)

These names came from the working files before the stages were organized. They map onto the stages above.

| Earlier name | Maps to | Notes |
|---|---|---|
| Working 1st | `history/working-1st/` | Working firmware and a two-file app (`pwa_firebase_bridge.js` and `safeair-app.html`). Mostly telemetry, with a few commands. Kept as a historical reference, not a stage |
| `rcode.zip` | V1 | Send-only backend skeleton |
| `Front-end-back-end.zip` | V2 | Web app with back functionality, no PWA |
| `PWA try two.zip` | V3 | PWA with the bridge merged into one HTML file |
| `Main.zip` | `firmware_final/main/` | Final firmware |

---

## The core tradeoff

- **No listeners (V1)**: lowest latency and simplest, but read-only.
- **Listeners (V2–V4)**: remote control, at the cost of sync latency and bandwidth.
- **PWA (V3–V4)**: installable and offline-capable, at the cost of extra assets. V3 paid a maintainability cost for its single-file HTML, and V4 removes it.

**Rule of thumb:** device ↔ cloud always goes through Firebase RTDB. The app **listens** for telemetry and **writes** command nodes. The firmware **listens** for command nodes and **writes** telemetry.

---

## Recommended combination (the complete system)

A working SafeAir deployment has **two parts**:

1. **Firmware:** Flash `firmware_final/main/` to your ESP32. Fill in your Firebase credentials.
2. **Web App:** Host `v4_modular_bridge/` (once added) or `v3_pwa_monolith/` (now). Fill in the same Firebase credentials.
3. **Optional backend:** Grow `v1_send_only/`'s backend skeleton into a secure middleman if you want to hide Firebase credentials from the web app.

The firmware and web app communicate through Firebase RTDB using the bidirectional schema introduced in V2.

## V1 — Send-Only Pipeline

**Folder:** `v1_send_only/`

**What it could do**
- Send sensor telemetry from the device to the cloud with almost no delay.
- Provide the full backend layout: data paths, rules, and Cloud Functions skeleton.

**What it gave up**
- **No Back-Functionality.** The app cannot command the device.
- Not a finished product on its own.

**Why it mattered**
Listening for commands needs a live connection, and that connection adds latency. V1 skipped it, which made it fast but one-way. It also set out the backend structure that the later stages were built on.

**Best for:** a fast, read-only monitor, or a template for future backend work.

---

## V2 — Bidirectional Web App

**Folder:** `v2_back_functionality/`

**What it could do**
- Show live readings.
- Send mute, test, arm, disarm, and pump commands to the device, with confirmations.

**What it gave up**
- No PWA. It cannot be installed on a phone home screen.
- Works only while the page is open in a browser.
- Two-way listeners add sync latency and bandwidth.

**Why it mattered**
This was the first version where the two-way control loop worked end to end. Everything after it reuses that loop.

**Best for:** a quick control panel on a laptop or a browser tab.

---

## V3 — PWA, Merged

**Folder:** `v3_pwa_monolith/`

**What it could do**
- Everything V2 could do.
- Install on a phone as a Progressive Web App, with a manifest and service worker.

**What it gave up**
- **Maintainability.** The bridge logic is merged into `safeair-app.html`, so a change to one part can break the other.
- Secrets were also merged into the same HTML file. The live Firebase config must stay out of any committed copy.

**Why it mattered**
It proved the installable phone app worked, and it showed clearly why the bridge should be separated.

**Best for:** a single-file PWA when you want ready-made assets in one place.

---

## V4 — PWA, Modular Bridge

**Folder:** `v4_modular_bridge/` (not yet added to the project)

**What it could do**
- Everything V3 could do.
- Keep the Firebase logic in `pwa_firebase_bridge.js`, separate from the UI in the app HTML.
- Keep the Firebase config in one place, where it can be git-ignored.

**What it gave up**
- The manifest and service worker must be assembled from V3 if their files are missing from this folder.

**Why it mattered**
This is the cleanest web stack. The UI and the cloud logic can change independently, and secrets stay out of the UI file.

**Best for:** the recommended app for production use, once the folder is in place.

---

## Firmware Final

**Folder:** `firmware_final/main/` (open `main.ino` in Arduino IDE)

**What it can do**
- Full sensor monitoring with a three-level alarm (SAFE, WARNING, DANGER).
- Automatic and manual suppression.
- Wi-Fi setup with no hard-coded credentials, through a built-in hotspot.
- Live telemetry, heartbeats, alerts, and full Back-Functionality with acknowledgements.

**Managers:** `WiFiManager`, `FirebaseManager`, `DeviceManager`, `PumpManager`, `SensorManager`, `DisplayManager`, `AlarmManager`, `ButtonManager`, `StorageManager`, `Logger`, with `JSONManager` building the payloads.

**Known limits**
- Database rules are open for the prototype stage. Authentication and App Check are on the roadmap.
- Wi-Fi and Firebase were the hardest parts to get stable. See the lessons in the README.

**Best for:** any SafeAir build, or a starting point for another smart device. See [Turning Anything Into a Smart Device](BUILD_A_SMART_DEVICE.md).

---

## Working 1st (before V1)

**Folder:** `history/working-1st/` (kept as a historical reference, not a stage)

**What it could do**
- Work firmware and a two-file app (`pwa_firebase_bridge.js` and `safeair-app.html`).
- Show telemetry live on a phone.

**What it gave up**
- Mostly telemetry. Only a few commands worked, so Back-Functionality was partial.
- Everything was in simple files, so Wi-Fi setup and cloud code were tangled together.

**Why it mattered**
It proved that a cheap microcontroller could publish safety data to the cloud and show it on a phone.

---

## Lessons across versions

- **Wi-Fi on an ESP32** needs a clear switch between access-point and station mode, and one owner for the network state.
- **Firebase streams and writes** should use separate objects, so the long-lived listener never shares a connection with telemetry uploads.
- **FreeRTOS tasks** need one owner for each piece of shared state, or they race each other.
- **Silent failures** are common on embedded hardware, so the logger and serial output matter as much as the code.

---

## Using the history

- To **run SafeAir**, use `firmware_final/main/` with `v4_modular_bridge/` once it's added, or `v3_pwa_monolith/` until then.
- To **study a web app**, look at V2 (the simplest) or V3 (the merged PWA).
- To **start a new backend**, look at V1.
- To **see where it started**, look at `history/working-1st/`.

Each stage has a git tag (for example `v2`) so the exact code can be restored at any time.
