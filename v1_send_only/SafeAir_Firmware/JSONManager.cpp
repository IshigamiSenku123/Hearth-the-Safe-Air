#include "JSONManager.h"

size_t JSONManager::buildTelemetry(char* buf, size_t len,
                                   const SensorSnapshot& snap,
                                   AlarmState alarm,
                                   const char* deviceId,
                                   uint32_t uptimeSec,
                                   int rssi) {
  StaticJsonDocument<512> doc;
  doc["ts"] = millis();
  doc["deviceId"] = deviceId;
  doc["uptime"] = uptimeSec;
  doc["rssi"] = rssi;
  doc["alarmState"] = (alarm == AlarmState::SAFE) ? "safe" :
                      (alarm == AlarmState::WARNING) ? "warning" : "danger";

  JsonObject sensors = doc.createNestedObject("sensors");
  sensors["lpg"]      = snap.lpg.valid      ? (float)round(snap.lpg.value)      : 0.0f;
  sensors["methane"]  = snap.methane.valid  ? (float)round(snap.methane.value)  : 0.0f;
  sensors["temp"]     = snap.temp.valid     ? (float)(round(snap.temp.value * 10) / 10.0) : 0.0f;
  sensors["smoke"]    = snap.smoke.valid    ? (float)round(snap.smoke.value)    : 0.0f;

  return serializeJson(doc, buf, len);
}

size_t JSONManager::buildHeartbeat(char* buf, size_t len,
                                   const char* deviceId,
                                   const char* fwVer,
                                   const char* ip,
                                   const char* ssid,
                                   uint32_t uptimeSec) {
  StaticJsonDocument<256> doc;
  doc["ts"] = millis();
  doc["deviceId"] = deviceId;
  doc["fw"] = fwVer;
  doc["ip"] = ip;
  doc["ssid"] = ssid;
  doc["uptime"] = uptimeSec;
  doc["freeHeap"] = ESP.getFreeHeap();
  return serializeJson(doc, buf, len);
}

size_t JSONManager::buildAck(char* buf, size_t len,
                             const char* cmdId,
                             bool success,
                             const char* message) {
  StaticJsonDocument<256> doc;
  doc["ts"] = millis();
  doc["cmdId"] = cmdId;
  doc["status"] = success ? "success" : "failure";
  doc["message"] = message;
  return serializeJson(doc, buf, len);
}

size_t JSONManager::buildLogBatch(char* buf, size_t len,
                                  const Logger& logger,
                                  size_t maxEvents) {
  StaticJsonDocument<1024> doc;
  JsonArray arr = doc.createNestedArray("events");
  size_t n = min(logger.count(), maxEvents);
  for (size_t i = 0; i < n; i++) {
    const LogEvent* e = logger.get(i);
    if (!e) break;
    JsonObject o = arr.createNestedObject();
    o["t"] = e->timestamp;
    o["lvl"] = (int)e->level;
    o["cat"] = (int)e->cat;
    o["title"] = e->title;
    o["msg"] = e->message;
    o["ack"] = e->ack;
  }
  return serializeJson(doc, buf, len);
}

JSONManager::Command JSONManager::parseCommand(const char* json) {
  Command cmd{};
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) return cmd;

  const char* id = doc["id"] | doc["cmdId"];
  const char* type = doc["type"] | doc["cmd"];
  if (!id || !type) return cmd;

  strlcpy(cmd.id, id, sizeof(cmd.id));
  strlcpy(cmd.type, type, sizeof(cmd.type));

  JsonObject payload = doc["payload"];
  if (payload) {
    serializeJson(payload, cmd.payload, sizeof(cmd.payload));
  } else {
    const char* raw = doc["payload"] | "";
    strlcpy(cmd.payload, raw, sizeof(cmd.payload));
  }
  cmd.valid = true;
  return cmd;
}