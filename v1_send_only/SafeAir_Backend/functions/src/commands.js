const functions = require("firebase-functions");
const { getDatabase } = require("firebase-admin/database");

// ---------------------------------------------------------------------------
// On ack: delete the command that was acknowledged
// ---------------------------------------------------------------------------
exports.onCommandAck = functions.database
  .ref("/devices/{deviceId}/ack/{pushId}")
  .onCreate(async (snap, context) => {
    const { deviceId } = context.params;
    const ack = snap.val();
    if (!ack || !ack.cmdId) return;

    const db = getDatabase();
    const cmdRef = db.ref(`/devices/${deviceId}/commands`).orderByChild("id").equalTo(ack.cmdId);
    const cmdSnap = await cmdRef.once("value");
    cmdSnap.forEach(child => {
      child.ref.remove();
      console.log(`Deleted executed command ${ack.cmdId} on ${deviceId}`);
    });
  });

// ---------------------------------------------------------------------------
// HTTP: provision a new device
// POST /provisionDevice  body: { deviceName, roomName }
// Returns: { deviceId, qrPayload }
// ---------------------------------------------------------------------------
exports.provisionDevice = functions.https.onRequest(async (req, res) => {
  if (req.method !== "POST") {
    res.status(405).send("Method Not Allowed");
    return;
  }

  const { deviceName = "SafeAir", roomName = "Kitchen" } = req.body || {};
  const mac = req.body.mac || generateDeviceId();
  const deviceId = mac.replace(/:/g, "").toLowerCase().slice(-8);

  const db = getDatabase();
  const ref = db.ref(`/devices/${deviceId}/meta/info`);
  await ref.set({
    deviceName,
    roomName,
    model: "ESP32-S3-N16R8",
    createdAt: Date.now(),
    provisioned: true
  });

  // QR payload = deviceId + secret (MVP: deviceId only. Phase 2: signed JWT)
  const qrPayload = `safeair:${deviceId}`;

  res.json({ deviceId, qrPayload, deviceName, roomName });
});

function generateDeviceId() {
  return "sa" + Math.random().toString(36).substring(2, 10);
}
