const functions = require("firebase-functions");
const { getDatabase } = require("firebase-admin/database");

// Thresholds loaded from device meta or hardcoded defaults
const DEFAULT_THRESHOLDS = {
  mq2_danger: 800, mq2_warn: 300,
  mq4_danger: 1000, mq4_warn: 400,
  tmp_danger: 60,   tmp_warn: 45,
  smk_danger: 500,  smk_warn: 200
};

exports.onTelemetry = functions.database
  .ref("/devices/{deviceId}/telemetry")
  .onWrite(async (change, context) => {
    const { deviceId } = context.params;
    const after = change.after.val();
    if (!after) return; // deletion

    const db = getDatabase();
    const root = db.ref(`/devices/${deviceId}`);

    // 1. Load thresholds (device meta overrides defaults)
    const thrSnap = await root.child("meta/thresholds").once("value");
    const thr = { ...DEFAULT_THRESHOLDS, ...(thrSnap.val() || {}) };

    // 2. Evaluate sensors
    const s = after.sensors || {};
    let level = "safe";
    let triggeredSensor = null;

    if ((s.lpg      || 0) >= thr.mq2_danger) { level = "danger"; triggeredSensor = "lpg"; }
    else if ((s.methane  || 0) >= thr.mq4_danger) { level = "danger"; triggeredSensor = "methane"; }
    else if ((s.temp     || 0) >= thr.tmp_danger) { level = "danger"; triggeredSensor = "temp"; }
    else if ((s.smoke    || 0) >= thr.smk_danger) { level = "danger"; triggeredSensor = "smoke"; }
    else if ((s.lpg      || 0) >= thr.mq2_warn)   { level = "warning"; triggeredSensor = "lpg"; }
    else if ((s.methane  || 0) >= thr.mq4_warn)   { level = "warning"; triggeredSensor = "methane"; }
    else if ((s.temp     || 0) >= thr.tmp_warn)   { level = "warning"; triggeredSensor = "temp"; }
    else if ((s.smoke    || 0) >= thr.smk_warn)   { level = "warning"; triggeredSensor = "smoke"; }

    // 3. Update derived state
    await root.child("meta/lastState").set({
      level,
      triggeredSensor,
      ts: after.ts || Date.now(),
      rssi: after.rssi || 0
    });

    // 4. If danger, write alert (dedup: only if last alert was >60s ago or different sensor)
    if (level === "danger") {
      const lastAlertSnap = await root.child("meta/lastAlert").once("value");
      const last = lastAlertSnap.val();
      const now = Date.now();
      if (!last || (now - last.ts > 60000) || last.sensor !== triggeredSensor) {
        const alertRef = root.child("alerts").push();
        await alertRef.set({
          level: "danger",
          sensor: triggeredSensor,
          value: s[triggeredSensor],
          ts: now,
          deviceId,
          acknowledged: false
        });
        await root.child("meta/lastAlert").set({ ts: now, sensor: triggeredSensor });
      }
    }

    // 5. If state improved from danger→safe, optionally log recovery
    const prev = change.before.val();
    if (prev && prev.alarmState === "danger" && after.alarmState === "safe") {
      await root.child("alerts").push().set({
        level: "recovery",
        sensor: "all",
        ts: Date.now(),
        deviceId,
        message: "All sensors returned to safe levels"
      });
    }
  });
