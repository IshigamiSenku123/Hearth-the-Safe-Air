#include "DeviceManager.h"
#include "WiFiManager.h"
#include "FirebaseManager.h"

DeviceManager Device;

bool DeviceManager::begin() {
  Storage.loadMeta(_meta);
  
  // FORCE device name to match web app subscription
  strncpy(_meta.deviceName, "kitchen-1", sizeof(_meta.deviceName) - 1);
  _meta.deviceName[sizeof(_meta.deviceName) - 1] = '\0';
  
  _meta.bootCount = Storage.incrementBootCount();
  Storage.saveMeta(_meta);  // save back so it persists
  _startMs = millis();

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

  if (strcmp(cmd.type, "arm") == 0) {
    Alarm.mute(false);
  } else if (strcmp(cmd.type, "disarm") == 0) {
    Alarm.mute(true);
  } else if (strcmp(cmd.type, "mute") == 0) {
    Alarm.mute(true);
  } else if (strcmp(cmd.type, "test") == 0) {
    Alarm.testAlarm(5000);
  } else if (strcmp(cmd.type, "reset") == 0) {
    factoryReset();
    strcpy(ackMsg, "factory reset triggered");
  } else if (strcmp(cmd.type, "pump") == 0) {
    Pump.trigger();
  } else if (strcmp(cmd.type, "update_threshold") == 0) {
    strcpy(ackMsg, "threshold update stub");
  } else {
    ok = false;
    strcpy(ackMsg, "unknown command");
  }

  size_t n = JSONManager::buildAck(_jsonBuf, sizeof(_jsonBuf), cmd.id, ok, ackMsg);
  if (n) Cloud.sendAck(_jsonBuf);

  SysLog.log(LogLevel::INFO, LogCategory::CLOUD,
    "Command executed", "%s -> %s", cmd.type, ok ? "ok" : "fail");
}

void DeviceManager::factoryReset() {
  Storage.clearWiFi();
  Storage.putString("cal_done", "false");
  SysLog.log(LogLevel::CRITICAL, LogCategory::SYSTEM,
    "Factory reset", "NVS cleared. Rebooting...");
  delay(100);
  ESP.restart();
}