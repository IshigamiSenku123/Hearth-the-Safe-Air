const functions = require("firebase-functions");
const { getDatabase } = require("firebase-admin/database");

// ---------------------------------------------------------------------------
// Scheduled: every hour, archive last hour of telemetry into /history
// ---------------------------------------------------------------------------
exports.archiveTelemetry = functions.pubsub
  .schedule("every 60 minutes")
  .onRun(async (context) => {
    const db = getDatabase();
    const devicesSnap = await db.ref("/devices").once("value");
    const devices = devicesSnap.val() || {};
    const now = Date.now();
    const oneHourAgo = now - 3600000;
    const dateStr = new Date(now).toISOString().split("T")[0]; // YYYY-MM-DD

    const updates = {};
    let archivedCount = 0;

    for (const [deviceId, devData] of Object.entries(devices)) {
      const telem = devData.telemetry;
      if (!telem || !telem.ts || telem.ts < oneHourAgo) continue;

      const path = `/history/${deviceId}/${dateStr}/${telem.ts}`;
      updates[path] = telem;
      archivedCount++;
    }

    if (archivedCount > 0) {
      await db.ref().update(updates);
      console.log(`Archived ${archivedCount} telemetry records to /history`);
    }
    return null;
  });
