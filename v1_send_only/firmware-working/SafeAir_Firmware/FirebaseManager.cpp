#include "FirebaseManager.h"

FirebaseManager Cloud;

static FirebaseManager* _fbInstance = nullptr;

bool FirebaseManager::begin(const char* host, const char* apiKey, const char* deviceId) {
  _fbInstance = this;
  snprintf(_devicePath, sizeof(_devicePath), "%s%s", FB_DEVICE_PATH, deviceId);
  snprintf(_cmdPath,    sizeof(_cmdPath),    "%s/commands", _devicePath);

  _config.database_url = host;
  _config.api_key = apiKey;

  // MVP: no auth token set — requires RTDB rules to be public (read/write: true)
  // Phase 2: add email/password or legacy token here
  Firebase.begin(&_config, &_auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.beginStream(_fbData, _cmdPath)) {
    Firebase.setStreamCallback(_fbData, _streamCallback, _streamTimeoutCallback);
    _ready = true;
    SysLog.log(LogLevel::INFO, LogCategory::CLOUD,
      "Firebase ready", "Path=%s", _devicePath);
  } else {
    SysLog.log(LogLevel::ERROR, LogCategory::CLOUD,
      "Firebase init failed", "%s", _fbData.errorReason().c_str());
  }
  return _ready;
}

void FirebaseManager::update() {
  if (!_ready) return;
  if (!Net.isConnected()) {
    _ready = false;
    SysLog.log(LogLevel::WARNING, LogCategory::CLOUD,
      "Cloud paused", "WiFi disconnected");
  }
}

void FirebaseManager::pushTelemetry(const char* json) {
  if (!_ready) return;
  String path = String(_devicePath) + "/telemetry";
  FirebaseJson fbJson;
  fbJson.setJsonData(json);
  Firebase.setJSON(_fbData, path.c_str(), fbJson);
}

void FirebaseManager::pushHeartbeat(const char* json) {
  if (!_ready) return;
  String path = String(_devicePath) + "/heartbeat";
  FirebaseJson fbJson;
  fbJson.setJsonData(json);
  Firebase.setJSON(_fbData, path.c_str(), fbJson);
}

void FirebaseManager::sendAck(const char* json) {
  if (!_ready) return;
  String path = String(_devicePath) + "/ack";
  FirebaseJson fbJson;
  fbJson.setJsonData(json);
  Firebase.pushJSON(_fbData, path.c_str(), fbJson);
}

void FirebaseManager::uploadLogs(const char* json) {
  if (!_ready) return;
  String path = String(_devicePath) + "/logs";
  FirebaseJson fbJson;
  fbJson.setJsonData(json);
  Firebase.pushJSON(_fbData, path.c_str(), fbJson);
}

JSONManager::Command FirebaseManager::getCommand() {
  _cmdPending = false;
  return _lastCmd;
}

void FirebaseManager::_streamCallback(StreamData data) {
  if (!_fbInstance) return;
  String json = data.stringData();
  JSONManager::Command cmd = JSONManager::parseCommand(json.c_str());
  if (cmd.valid) {
    _fbInstance->_lastCmd = cmd;
    _fbInstance->_cmdPending = true;
    SysLog.log(LogLevel::INFO, LogCategory::CLOUD,
      "Command received", "type=%s id=%s", cmd.type, cmd.id);
  }
}

void FirebaseManager::_streamTimeoutCallback(bool timeout) {
  if (timeout) {
    SysLog.log(LogLevel::WARNING, LogCategory::CLOUD,
      "Stream timeout", "Reconnecting...");
  }
}