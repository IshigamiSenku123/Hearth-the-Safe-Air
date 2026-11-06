#include "DeviceManager.h"
#include "WiFiManager.h"
#include "FirebaseManager.h"
#include <ArduinoJson.h>

// Global thresholds declared in SafeAir_Firmware.ino
extern ThresholdSet thresholds;

DeviceManager Device;

bool DeviceManager::begin() {
  Storage.loadMeta(_meta);
  
  // FORCE device name to match PWA subscription
  strncpy(_meta.deviceName, "kitchen-1", sizeof(_meta.deviceName) - 1);
  _meta.deviceName[sizeof(_meta.deviceName) - 1] = '\0';
  
  _meta.bootCount = Storage.incrementBootCount();
  Storage.saveMeta(_meta);
  _startMs = millis();

  Serial.print("[DEVICE] Booted as: ");
  Serial.println(_meta.deviceName);
  Serial.print("[DEVICE] FW version: ");
  Serial.println(SAFEAIR_FW_VERSION);
  Serial.print("[DEVICE] Boot count: ");
  Serial.println(_meta.bootCount);

  SysLog.log(LogLevel::INFO, LogCategory::SYSTEM,
    "Device boot", "%s #%lu FW=%s", _meta.deviceName, _meta.bootCount, SAFEAIR_FW_VERSION);
  return true;
}

void DeviceManager::update() {
  uint32_t now = millis();

  if (now - _lastTelem >= INTERVAL_CLOUD_MS) {
    _lastTelem = now;
    _sendTelemetry();
  }

  if (now - _lastHb >= INTERVAL_HEARTBEAT_MS) {
    _lastHb = now;
    _sendHeartbeat();
  }
}

void DeviceManager::_sendTelemetry() {
  if (!Net.isConnected() || !Cloud.isReady()) return;
  const SensorSnapshot& snap = Sensors.read();
  AlarmState st = Alarm.state();
  int rssi = Net.isConnected() ? Net.rssi() : 0;

  size_t n = JSONManager::buildTelemetry(_jsonBuf, sizeof(_jsonBuf),
    snap, st, _meta.deviceName, uptime(), rssi);
  if (n) Cloud.pushTelemetry(_jsonBuf);
}

void DeviceManager::_sendHeartbeat() {
  if (!Net.isConnected() || !Cloud.isReady()) return;
  size_t n = JSONManager::buildHeartbeat(_jsonBuf, sizeof(_jsonBuf),
    _meta.deviceName, SAFEAIR_FW_VERSION,
    Net.localIP().c_str(), Net.ssid().c_str(), uptime());
  if (n) Cloud.pushHeartbeat(_jsonBuf);
}

void DeviceManager::onCommand(const JSONManager::Command& cmd) {
  bool ok = true;
  char ackMsg[64] = "done";

  Serial.print("[CMD] Received: type=");
  Serial.print(cmd.type);
  Serial.print(" id=");
  Serial.println(cmd.id);
  if (strlen(cmd.payload) > 0) {
    Serial.print("[CMD] Payload: ");
    Serial.println(cmd.payload);
  }

  if (strcmp(cmd.type, "arm") == 0) {
    Alarm.mute(false);
    Serial.println("[CMD] ACTION: Armed (unmuted)");
    SysLog.log(LogLevel::INFO, LogCategory::CLOUD, "Cmd", "Armed");

  } else if (strcmp(cmd.type, "disarm") == 0) {
    Alarm.mute(true);
    Serial.println("[CMD] ACTION: Disarmed (muted)");
    SysLog.log(LogLevel::INFO, LogCategory::CLOUD, "Cmd", "Disarmed");

  } else if (strcmp(cmd.type, "mute") == 0) {
    Alarm.mute(true);
    Serial.println("[CMD] ACTION: Muted");
    SysLog.log(LogLevel::INFO, LogCategory::CLOUD, "Cmd", "Muted");

  } else if (strcmp(cmd.type, "unmute") == 0) {
    Alarm.mute(false);
    Serial.println("[CMD] ACTION: Unmuted");
    SysLog.log(LogLevel::INFO, LogCategory::CLOUD, "Cmd", "Unmuted");

  } else if (strcmp(cmd.type, "test") == 0) {
    Alarm.testAlarm(5000);
    Serial.println("[CMD] ACTION: Test alarm triggered for 5s");
    SysLog.log(LogLevel::INFO, LogCategory::CLOUD, "Cmd", "Test alarm");

  } else if (strcmp(cmd.type, "reset") == 0) {
    Serial.println("[CMD] ACTION: Factory reset triggered");
    SysLog.log(LogLevel::INFO, LogCategory::CLOUD, "Cmd", "Factory reset");
    factoryReset();
    return;  // rebooting

  } else if (strcmp(cmd.type, "pump") == 0 || strcmp(cmd.type, "pump_arm") == 0) {
    Pump.trigger();
    strcpy(ackMsg, "pump ON");
    Serial.println("[CMD] ACTION: Pump ON");
    SysLog.log(LogLevel::INFO, LogCategory::CLOUD, "Cmd", "Pump ON");

  } else if (strcmp(cmd.type, "pump_disarm") == 0) {
    // Add Pump.stop() here if your PumpManager has it
    strcpy(ackMsg, "pump OFF");
    Serial.println("[CMD] ACTION: Pump OFF");
    SysLog.log(LogLevel::INFO, LogCategory::CLOUD, "Cmd", "Pump OFF");

  } else if (strcmp(cmd.type, "update_threshold") == 0) {
    Serial.println("[CMD] Parsing threshold update...");
    StaticJsonDocument<256> pdoc;
    DeserializationError err = deserializeJson(pdoc, cmd.payload);
    if (!err) {
      // Match ThresholdSet struct field names exactly
      thresholds.mq2_warn   = pdoc["mq2_w"] | thresholds.mq2_warn;
      thresholds.mq2_danger = pdoc["mq2_d"] | thresholds.mq2_danger;
      thresholds.mq4_warn   = pdoc["mq4_w"] | thresholds.mq4_warn;
      thresholds.mq4_danger = pdoc["mq4_d"] | thresholds.mq4_danger;
      thresholds.temp_warn  = pdoc["tmp_w"] | thresholds.temp_warn;
      thresholds.temp_danger= pdoc["tmp_d"] | thresholds.temp_danger;
      thresholds.smoke_warn = pdoc["smk_w"] | thresholds.smoke_warn;
      thresholds.smoke_danger = pdoc["smk_d"] | thresholds.smoke_danger;

      Storage.saveThresholds(thresholds);
      strcpy(ackMsg, "thresholds saved");

      Serial.println("[THRESHOLD] UPDATED AND SAVED TO NVS:");
      Serial.print("  MQ2 (LPG):    warn="); Serial.print(thresholds.mq2_warn); 
      Serial.print(" danger="); Serial.println(thresholds.mq2_danger);
      Serial.print("  MQ4 (Methane):warn="); Serial.print(thresholds.mq4_warn); 
      Serial.print(" danger="); Serial.println(thresholds.mq4_danger);
      Serial.print("  Temp:         warn="); Serial.print(thresholds.temp_warn); 
      Serial.print(" danger="); Serial.println(thresholds.temp_danger);
      Serial.print("  Smoke:        warn="); Serial.print(thresholds.smoke_warn); 
      Serial.print(" danger="); Serial.println(thresholds.smoke_danger);

      SysLog.log(LogLevel::INFO, LogCategory::CLOUD,
        "Thresholds updated", "mq2=%d/%d mq4=%d/%d tmp=%d/%d smk=%d/%d",
        thresholds.mq2_warn, thresholds.mq2_danger,
        thresholds.mq4_warn, thresholds.mq4_danger,
        thresholds.temp_warn, thresholds.temp_danger,
        thresholds.smoke_warn, thresholds.smoke_danger);
    } else {
      ok = false;
      strcpy(ackMsg, "bad threshold JSON");
      Serial.print("[THRESHOLD] ERROR: Failed to parse JSON: ");
      Serial.println(err.c_str());
    }
  } else {
    ok = false;
    strcpy(ackMsg, "unknown command");
    Serial.print("[CMD] ERROR: Unknown command type: ");
    Serial.println(cmd.type);
  }

  size_t n = JSONManager::buildAck(_jsonBuf, sizeof(_jsonBuf), cmd.id, ok, ackMsg);
  if (n) {
    Cloud.sendAck(_jsonBuf);
    Serial.print("[CMD] ACK sent: ");
    Serial.println(ackMsg);
  }

  SysLog.log(LogLevel::INFO, LogCategory::CLOUD,
    "Command done", "%s -> %s", cmd.type, ok ? "ok" : "fail");
}

void DeviceManager::factoryReset() {
  Serial.println("[SYSTEM] Factory reset: clearing NVS...");
  Storage.clearWiFi();
  Storage.putString("cal_done", "false");
  SysLog.log(LogLevel::CRITICAL, LogCategory::SYSTEM,
    "Factory reset", "NVS cleared. Rebooting...");
  delay(100);
  ESP.restart();
}