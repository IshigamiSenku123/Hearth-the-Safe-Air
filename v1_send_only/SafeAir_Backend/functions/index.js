const { initializeApp } = require("firebase-admin/app");
initializeApp();

const telemetry = require("./src/telemetryProcessor");
const notify    = require("./src/notifications");
const cmd       = require("./src/commands");
const archive   = require("./src/historyArchiver");

// ---------------------------------------------------------------------------
// Trigger: new telemetry arrives → evaluate thresholds → alert + notify
// ---------------------------------------------------------------------------
exports.onTelemetry = telemetry.onTelemetry;

// ---------------------------------------------------------------------------
// Trigger: new danger alert → send FCM push to all subscribed tokens
// ---------------------------------------------------------------------------
exports.onDangerAlert = notify.onDangerAlert;

// ---------------------------------------------------------------------------
// Trigger: command ack received → clean up old commands
// ---------------------------------------------------------------------------
exports.onCommandAck = cmd.onCommandAck;

// ---------------------------------------------------------------------------
// Scheduled: archive telemetry to /history/{deviceId}/{date} every hour
// ---------------------------------------------------------------------------
exports.archiveTelemetry = archive.archiveTelemetry;

// ---------------------------------------------------------------------------
// HTTP: device provisioning endpoint (returns deviceId + QR payload)
// ---------------------------------------------------------------------------
exports.provisionDevice = cmd.provisionDevice;
