#include "FirebaseManager.h"

FirebaseManager Cloud;

bool FirebaseManager::begin(const char* host, const char* apiKey, const char* deviceId) {
  snprintf(_devicePath, sizeof(_devicePath), "%s%s", FB_DEVICE_PATH, deviceId);
  snprintf(_cmdPath, sizeof(_cmdPath), "%s/commands", _devicePath);
  strncpy(_host, host, sizeof(_host) - 1);
  
  // Skip SSL certificate validation (test mode / open RTDB rules)
  _client.setInsecure();
  _client.setTimeout(5000);
  
  _ready = true;
  SysLog.log(LogLevel::INFO, LogCategory::CLOUD,
    "Firebase ready", "Path=%s", _devicePath);
  return _ready;
}

void FirebaseManager::update() {
  if (!_ready) return;
  
  if (!Net.isConnected()) {
    if (_ready) {
      _ready = false;
      SysLog.log(LogLevel::WARNING, LogCategory::CLOUD,
        "Cloud paused", "WiFi disconnected");
    }
    return;
  }
  
  if (!_ready && Net.isConnected()) {
    _ready = true;
    SysLog.log(LogLevel::INFO, LogCategory::CLOUD,
      "Firebase ready", "Reconnected");
  }
  
  // Poll for commands every 1 second via HTTP GET
  unsigned long now = millis();
  if (now - _lastCmdPoll >= CMD_POLL_INTERVAL) {
    _lastCmdPoll = now;
    String json = _httpGet(_cmdPath);
    if (json.length() > 0 && json != "null" && json != "null\n") {
      JSONManager::Command cmd = JSONManager::parseCommand(json.c_str());
      if (cmd.valid) {
        _lastCmd = cmd;
        _cmdPending = true;
        SysLog.log(LogLevel::INFO, LogCategory::CLOUD,
          "Command received", "type=%s id=%s", cmd.type, cmd.id);
      }
    }
  }
}

String FirebaseManager::_buildUrl(const char* path) {
  String url = "https://";
  url += _host;
  url += path;
  url += ".json";
  // If you add auth rules later, append ?auth=<token> here
  return url;
}

bool FirebaseManager::_httpPut(const char* path, const char* json) {
  if (!Net.isConnected()) return false;
  HTTPClient http;
  String url = _buildUrl(path);
  http.begin(_client, url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.PUT(json);
  bool success = (httpCode == 200 || httpCode == 204);
  if (!success) {
    Serial.print("[FIREBASE] HTTP PUT failed, code: ");
    Serial.println(httpCode);
  }
  http.end();
  return success;
}

bool FirebaseManager::_httpPost(const char* path, const char* json) {
  if (!Net.isConnected()) return false;
  HTTPClient http;
  String url = _buildUrl(path);
  http.begin(_client, url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(json);
  bool success = (httpCode == 200 || httpCode == 204);
  if (!success) {
    Serial.print("[FIREBASE] HTTP POST failed, code: ");
    Serial.println(httpCode);
  }
  http.end();
  return success;
}

String FirebaseManager::_httpGet(const char* path) {
  if (!Net.isConnected()) return "";
  HTTPClient http;
  String url = _buildUrl(path);
  http.begin(_client, url);
  int httpCode = http.GET();
  String payload = "";
  if (httpCode == 200) {
    payload = http.getString();
  }
  http.end();
  return payload;
}

void FirebaseManager::pushTelemetry(const char* json) {
  if (!_ready) return;
  String path = String(_devicePath) + "/telemetry";
  if (_httpPut(path.c_str(), json)) {
    Serial.println("[FIREBASE] Telemetry write OK");
  } else {
    Serial.println("[FIREBASE] Telemetry write FAILED");
  }
}

void FirebaseManager::pushHeartbeat(const char* json) {
  if (!_ready) return;
  String path = String(_devicePath) + "/heartbeat";
  if (!_httpPut(path.c_str(), json)) {
    Serial.println("[FIREBASE] Heartbeat write FAILED");
  }
}

void FirebaseManager::sendAck(const char* json) {
  if (!_ready) return;
  String path = String(_devicePath) + "/ack";
  _httpPost(path.c_str(), json);
}

void FirebaseManager::uploadLogs(const char* json) {
  if (!_ready) return;
  String path = String(_devicePath) + "/logs";
  _httpPost(path.c_str(), json);
}

JSONManager::Command FirebaseManager::getCommand() {
  _cmdPending = false;
  return _lastCmd;
}