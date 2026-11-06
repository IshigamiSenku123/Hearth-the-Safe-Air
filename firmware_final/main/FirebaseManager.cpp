#include "FirebaseManager.h"

FirebaseManager Cloud;

bool FirebaseManager::begin(const char* host, const char* apiKey, const char* deviceId) {
  snprintf(_devicePath, sizeof(_devicePath), "%s%s", FB_DEVICE_PATH, deviceId);
  snprintf(_cmdPath,    sizeof(_cmdPath),    "%s/commands", _devicePath);
  strncpy(_host, host, sizeof(_host) - 1);
  _client.setInsecure();
  _client.setTimeout(5000);
  _ready = true;
  SysLog.log(LogLevel::INFO, LogCategory::CLOUD, "Firebase ready", "Path=%s", _devicePath);
  return _ready;
}

void FirebaseManager::update() {
  if (!_ready || !Net.isConnected()) {
    if (_ready && !Net.isConnected()) _ready = false;
    return;
  }
  
  if (!_ready && Net.isConnected()) _ready = true;
  
  unsigned long now = millis();
  if (now - _lastCmdPoll >= CMD_POLL_INTERVAL) {
    _lastCmdPoll = now;
    String json = _httpGet(_cmdPath);
    
    if (json.length() == 0 || json == "null" || json == "null\n") return;
    
    // Try flat first
    _lastCmd = JSONManager::parseCommand(json.c_str());
    if (_lastCmd.valid) {
      _cmdPending = true;
      return;
    }
    
    // Try nested (PWA uses push())
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, json.c_str())) return;
    
    JsonObject root = doc.as<JsonObject>();
    for (JsonPair kv : root) {
      if (kv.value().is<JsonObject>()) {
        JsonObject cmdObj = kv.value();
        const char* type = cmdObj["type"] | cmdObj["cmd"];
        const char* id = cmdObj["id"] | cmdObj["cmdId"];
        if (type && id) {
          strlcpy(_lastCmd.type, type, sizeof(_lastCmd.type));
          strlcpy(_lastCmd.id, id, sizeof(_lastCmd.id));
          JsonObject payload = cmdObj["payload"];
          if (payload) {
            serializeJson(payload, _lastCmd.payload, sizeof(_lastCmd.payload));
          } else {
            const char* raw = cmdObj["payload"] | "";
            strlcpy(_lastCmd.payload, raw, sizeof(_lastCmd.payload));
          }
          _lastCmd.valid = true;
          _cmdPending = true;
          return;
        }
      }
    }
  }
}

String FirebaseManager::_buildUrl(const char* path) {
  String url = "https://";
  url += _host;
  url += path;
  url += ".json";
  return url;
}

bool FirebaseManager::_httpPut(const char* path, const char* json) {
  if (!Net.isConnected()) return false;
  HTTPClient http;
  http.begin(_client, _buildUrl(path));
  http.addHeader("Content-Type", "application/json");
  int code = http.PUT(json);
  http.end();
  return (code == 200 || code == 204);
}

bool FirebaseManager::_httpPost(const char* path, const char* json) {
  if (!Net.isConnected()) return false;
  HTTPClient http;
  http.begin(_client, _buildUrl(path));
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(json);
  http.end();
  return (code == 200 || code == 204);
}

bool FirebaseManager::_httpDelete(const char* path) {
  if (!Net.isConnected()) return false;
  HTTPClient http;
  http.begin(_client, _buildUrl(path));
  int code = http.sendRequest("DELETE");
  http.end();
  return (code == 200 || code == 204);
}

String FirebaseManager::_httpGet(const char* path) {
  if (!Net.isConnected()) return "";
  HTTPClient http;
  http.begin(_client, _buildUrl(path));
  int code = http.GET();
  String payload = (code == 200) ? http.getString() : "";
  http.end();
  return payload;
}

void FirebaseManager::pushTelemetry(const char* json) {
  if (!_ready) return;
  String path = String(_devicePath) + "/telemetry";
  _httpPut(path.c_str(), json);
}

void FirebaseManager::pushHeartbeat(const char* json) {
  if (!_ready) return;
  String path = String(_devicePath) + "/heartbeat";
  _httpPut(path.c_str(), json);
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

void FirebaseManager::clearCommands() {
  _httpDelete(_cmdPath);
}

JSONManager::Command FirebaseManager::getCommand() {
  _cmdPending = false;
  return _lastCmd;
}