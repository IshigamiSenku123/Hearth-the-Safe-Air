const functions = require("firebase-functions");
const { getMessaging } = require("firebase-admin/messaging");
const { getDatabase } = require("firebase-admin/database");

exports.onDangerAlert = functions.database
  .ref("/devices/{deviceId}/alerts/{alertId}")
  .onCreate(async (snap, context) => {
    const { deviceId, alertId } = context.params;
    const alert = snap.val();
    if (!alert || alert.level !== "danger") return;

    const db = getDatabase();
    const tokensSnap = await db.ref(`/devices/${deviceId}/meta/fcmTokens`).once("value");
    const tokensObj = tokensSnap.val() || {};
    const tokens = Object.keys(tokensObj).filter(t => tokensObj[t] === true);

    if (tokens.length === 0) {
      console.log(`No FCM tokens for ${deviceId}`);
      return;
    }

    const message = {
      notification: {
        title: "🚨 SafeAir ALERT",
        body: `${alert.sensor.toUpperCase()} danger detected on ${deviceId}. Value: ${alert.value}`
      },
      data: {
        deviceId,
        alertId,
        sensor: alert.sensor,
        value: String(alert.value),
        click_action: "FLUTTER_NOTIFICATION_CLICK"  // or your PWA route
      },
      tokens
    };

    try {
      const response = await getMessaging().sendEachForMulticast(message);
      console.log(`FCM sent: ${response.successCount} success, ${response.failureCount} failure`);
      // Remove invalid tokens
      response.responses.forEach((resp, idx) => {
        if (!resp.success && resp.error && (
          resp.error.code === "messaging/invalid-registration-token" ||
          resp.error.code === "messaging/registration-token-not-registered"
        )) {
          const badToken = tokens[idx];
          db.ref(`/devices/${deviceId}/meta/fcmTokens/${badToken}`).remove();
        }
      });
    } catch (err) {
      console.error("FCM send failed:", err);
    }
  });
