// SafeAir Firebase Bridge — Drop this into safeair.html
// Replaces simulated data with live RTDB data
// =============================================================================

// ---------------------------------------------------------------------------
// CONFIG: paste your Firebase config here (from Firebase Console)
// ---------------------------------------------------------------------------
const FIREBASE_CONFIG = {
  apiKey: "YOUR_FIREBASE_API_KEY",
  authDomain: "PASTE_YOUR_AUTH_DOMAIN",
  databaseURL: "YOUR_DATABASE_URL",
  projectId: "PASTE_YOUR_PROJECT_ID",
  storageBucket: "PASTE_YOUR_STORAGE_BUCKET",
  messagingSenderId: "PASTE_YOUR_SENDER_ID",
  appId: "PASTE_YOUR_APP_ID"
};

const DEVICE_ID = "kitchen-1";  // Must match deviceName in ESP32 NVS/Config

// ---------------------------------------------------------------------------
// Initialize Firebase (ES modules from CDN)
// ---------------------------------------------------------------------------
import { initializeApp } from "https://www.gstatic.com/firebasejs/10.12.0/firebase-app.js";
import {
  getDatabase, ref, onValue, set, push, child, onChildAdded, remove
} from "https://www.gstatic.com/firebasejs/10.12.0/firebase-database.js";

const app = initializeApp(FIREBASE_CONFIG);
const db = getDatabase(app);

// ---------------------------------------------------------------------------
// Live data subscription — replaces simulated sensor values
// ---------------------------------------------------------------------------
const telemRef = ref(db, `devices/${DEVICE_ID}/telemetry`);

onValue(telemRef, (snapshot) => {
  const data = snapshot.val();
  if (!data) {
    console.warn("[Firebase] No telemetry data yet");
    return;
  }

  // Map RTDB fields to your existing state.sensors structure
  if (state.sensors.lpg)    state.sensors.lpg.value    = data.sensors?.lpg    || 0;
  if (state.sensors.gas)   state.sensors.gas.value    = data.sensors?.methane || 0;
  if (state.sensors.smoke) state.sensors.smoke.value  = data.sensors?.smoke  || 0;
  if (state.sensors.temp)  state.sensors.temp.value   = data.sensors?.temp   || 0;

  // Update device online status
  const d = state.devices.find(x => x.id === DEVICE_ID);
  if (d) {
    d.online = true;
    d.rssi = data.rssi || 0;
    d.uptime = formatUptime(data.uptime || 0);
  }

  // Update alarm state
  const alarmMap = { safe: 'safe', warning: 'warning', danger: 'danger' };
  const newStatus = alarmMap[data.alarmState] || 'safe';
  if (newStatus !== lastHomeStatus) {
    lastHomeStatus = newStatus;
    // Trigger UI refresh
    if (typeof renderShell === 'function') renderShell();
  }

  // Refresh live view if open
  if (state.view === 'live' && typeof updateLive === 'function') {
    updateLive();
  }

  console.log("[Firebase] Telemetry updated", data.sensors);
});

// ---------------------------------------------------------------------------
// Heartbeat watcher — shows if device is online
// ---------------------------------------------------------------------------
const hbRef = ref(db, `devices/${DEVICE_ID}/heartbeat`);
onValue(hbRef, (snap) => {
  const hb = snap.val();
  if (!hb) return;
  const d = state.devices.find(x => x.id === DEVICE_ID);
  if (d) {
    d.online = true;
    d.fw = hb.fw || d.fw;
    d.uptime = formatUptime(hb.uptime || 0);
  }
});

// ---------------------------------------------------------------------------
// Ack watcher — show "command confirmed" toasts
// ---------------------------------------------------------------------------
const ackRef = ref(db, `devices/${DEVICE_ID}/ack`);
onChildAdded(ackRef, (snap) => {
  const ack = snap.val();
  if (!ack) return;
  toast(
    ack.status === 'success' ? 'Command confirmed' : 'Command failed',
    ack.message || ack.cmdId,
    ack.status === 'success' ? 'safe' : 'danger'
  );
  // Clean up old ack after showing
  setTimeout(() => remove(snap.ref), 5000);
});

// ---------------------------------------------------------------------------
// Command sender — call this from your UI buttons
// ---------------------------------------------------------------------------
window.sendDeviceCommand = function(type, payload = {}) {
  const cmdRef = ref(db, `devices/${DEVICE_ID}/commands`);
  const newCmd = push(cmdRef);
  set(newCmd, {
    id: newCmd.key,
    type: type,        // "mute", "unmute", "test", "arm", "disarm", "pump", "reset"
    ts: Date.now(),
    payload: payload
  });
  toast('Sending command...', type, 'info');
};

// ---------------------------------------------------------------------------
// Alert watcher — real-time danger notifications
// ---------------------------------------------------------------------------
const alertRef = ref(db, `devices/${DEVICE_ID}/alerts`);
onChildAdded(alertRef, (snap) => {
  const alert = snap.val();
  if (!alert || alert.level !== 'danger') return;
  toast(
    '🚨 DANGER: ' + alert.sensor.toUpperCase(),
    `Value: ${alert.value} on ${alert.deviceId}`,
    'danger'
  );
});

// ---------------------------------------------------------------------------
// Helper: format seconds into "4d 12h 45m"
// ---------------------------------------------------------------------------
function formatUptime(sec) {
  const d = Math.floor(sec / 86400);
  const h = Math.floor((sec % 86400) / 3600);
  const m = Math.floor((sec % 3600) / 60);
  const parts = [];
  if (d) parts.push(d + 'd');
  if (h) parts.push(h + 'h');
  if (m) parts.push(m + 'm');
  return parts.join(' ') || '0m';
}

// ---------------------------------------------------------------------------
// Wire existing UI buttons to real commands
// ---------------------------------------------------------------------------
// After your DOM is ready, override these handlers:
document.addEventListener('DOMContentLoaded', () => {
  // Mute button
  const origMute = document.querySelector('[data-action="alarm:silence"]');
  if (origMute) {
    origMute.addEventListener('click', () => {
      state.quiet = !state.quiet;
      sendDeviceCommand(state.quiet ? 'mute' : 'unmute');
    });
  }

  // Test alarm button
  const origTest = document.querySelector('[data-action="alarm:test"]');
  if (origTest) {
    origTest.addEventListener('click', () => {
      sendDeviceCommand('test');
    });
  }

  // Arm/disarm toggle
  const origArm = document.querySelector('[data-action="ctrl:arm"]');
  if (origArm) {
    origArm.addEventListener('click', () => {
      state.armed = !state.armed;
      sendDeviceCommand(state.armed ? 'arm' : 'disarm');
    });
  }

  // Pump toggle
  const origPump = document.querySelector('[data-action="supp:toggle"]');
  if (origPump) {
    origPump.addEventListener('click', () => {
      state.suppression = !state.suppression;
      sendDeviceCommand(state.suppression ? 'pump_arm' : 'pump_disarm');
    });
  }
});

console.log("[SafeAir Bridge] Firebase RTDB connected for device:", DEVICE_ID);
