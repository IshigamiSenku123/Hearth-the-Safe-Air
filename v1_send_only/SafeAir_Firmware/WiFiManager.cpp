#include "WiFiManager.h"

WiFiManager Net;

bool WiFiManager::begin() {
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);
  return _connectStored();   // true = creds found, false = AP started
}

void WiFiManager::update() {
  // AP mode: service web server
  if (_mode == WiFiMode::AP_MODE) {
    _server.handleClient();

    if (_provisioning && (millis() - _provStart > SOFTAP_TIMEOUT_MS)) {
      SysLog.log(LogLevel::WARNING, LogCategory::NETWORK,
        "AP timeout", "Provisioning window closed");
      stopProvisioning();
    }
    return;
  }

  if (_mode == WiFiMode::CONNECTED) {
    if (WiFi.status() != WL_CONNECTED) {
      SysLog.log(LogLevel::WARNING, LogCategory::NETWORK,
        "WiFi lost", "Attempting reconnect...");
      _mode = WiFiMode::DISCONNECTED;
      _retryCount = 0;
      _lastAttempt = 0;
    }
  }
  else if (_mode == WiFiMode::CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      _mode = WiFiMode::CONNECTED;
      _retryCount = 0;
      SysLog.log(LogLevel::INFO, LogCategory::NETWORK,
        "WiFi connected", "IP=%s RSSI=%d", localIP().c_str(), rssi());
    } else if (millis() - _lastAttempt > 500) {
      _lastAttempt = millis();
      if (++_retryCount > WIFI_MAX_RETRIES * 10) {
        SysLog.log(LogLevel::ERROR, LogCategory::NETWORK,
          "WiFi failed", "Max retries exceeded");
        _mode = WiFiMode::DISCONNECTED;
        _retryCount = 0;
      }
    }
  }
  else if (_mode == WiFiMode::DISCONNECTED) {
    if (millis() - _lastAttempt > WIFI_RECONNECT_MS) {
      _connectStored();
    }
  }
}

bool WiFiManager::_connectStored() {
  if (WiFi.status() == WL_CONNECTED) {
    _mode = WiFiMode::CONNECTED;
    return true;
  }

  WiFiCredentials cred;
  bool loaded = Storage.loadWiFi(cred);
  
  // DEBUG
  Serial.print("[DEBUG] loadWiFi returned: ");
  Serial.println(loaded ? "true" : "false");
  Serial.print("[DEBUG] cred.valid: ");
  Serial.println(cred.valid ? "true" : "false");
  if (loaded && cred.valid) {
    Serial.print("[DEBUG] SSID loaded: [");
    Serial.print(cred.ssid);
    Serial.println("]");
    Serial.print("[DEBUG] PASS loaded: [");
    Serial.print(cred.password);
    Serial.println("]");
  }
  // END DEBUG

  if (loaded && cred.valid) {
    // FIX: evict lingering AP state from previous ESP.restart()
    WiFi.softAPdisconnect(true);   // force AP off
    WiFi.disconnect();              // clear stale STA
    delay(100);                     // let radio settle
    WiFi.mode(WIFI_STA);
    WiFi.begin(cred.ssid, cred.password);
    _mode = WiFiMode::CONNECTING;
    _lastAttempt = millis();
    _retryCount = 0;
    SysLog.log(LogLevel::INFO, LogCategory::NETWORK,
      "WiFi connecting", "SSID=%s", cred.ssid);
    return true;
  }

  SysLog.log(LogLevel::INFO, LogCategory::NETWORK,
    "No WiFi creds", "Starting provisioning mode");
  _enterAPMode();
  return false;
}

void WiFiManager::_enterAPMode() {
  WiFi.mode(WIFI_AP);  // pure AP, no STA confusion
  String apName = String(SOFTAP_SSID_PREFIX) + String((uint32_t)ESP.getEfuseMac(), HEX);
  WiFi.softAP(apName.c_str(), SOFTAP_PASS);
  _mode = WiFiMode::AP_MODE;
  _provisioning = true;
  _provStart = millis();
  _startProvServer();
  SysLog.log(LogLevel::INFO, LogCategory::NETWORK,
    "AP started", "SSID=%s IP=%s", apName.c_str(),
    WiFi.softAPIP().toString().c_str());
}

void WiFiManager::startProvisioning() {
  if (!_provisioning) _enterAPMode();
}

void WiFiManager::stopProvisioning() {
  if (_provisioning) {
    WiFi.softAPdisconnect(true);
    _provisioning = false;
    _serverStarted = false;
    _connectStored();
  }
}

// -------------------- ADDED: Minimal provisioning server --------------------
void WiFiManager::_startProvServer() {
  if (_serverStarted) return;
  _serverStarted = true;

  _server.on("/", HTTP_GET, [this](){ _handleRoot(); });
  _server.on("/save", HTTP_POST, [this](){ _handleSave(); });
  _server.begin();

  SysLog.log(LogLevel::INFO, LogCategory::NETWORK,
    "Prov server up", "http://%s", WiFi.softAPIP().toString().c_str());
}

void WiFiManager::_handleRoot() {
  Serial.println("[PROV] GET / received");
  _server.send(200, "text/html",
    "<!doctype html><html><body>"
    "<h2>SafeAir WiFi Setup</h2>"
    "<form method='POST' action='/save'>"
    "SSID:<br><input name='ssid'><br><br>"
    "Password:<br><input type='password' name='pass'><br><br>"
    "<input type='submit' value='Connect'>"
    "</form></body></html>");
}

void WiFiManager::_handleSave() {
  Serial.println("[PROV] POST /save received");
  String ssid = _server.arg("ssid");
  String pass = _server.arg("pass");

  // DEBUG
  Serial.print("[DEBUG] Form SSID: [");
  Serial.print(ssid);
  Serial.println("]");
  Serial.print("[DEBUG] Form PASS: [");
  Serial.print(pass);
  Serial.println("]");

  if (ssid.length() > 0) {
    WiFiCredentials cred;
    strncpy(cred.ssid, ssid.c_str(), sizeof(cred.ssid) - 1);
    cred.ssid[sizeof(cred.ssid) - 1] = '\0';
    strncpy(cred.password, pass.c_str(), sizeof(cred.password) - 1);
    cred.password[sizeof(cred.password) - 1] = '\0';
    cred.valid = true;
    
    bool saved = Storage.saveWiFi(cred);
    Serial.print("[DEBUG] saveWiFi returned: ");
    Serial.println(saved ? "true" : "false");
    
    // Verify immediately
    WiFiCredentials verify;
    bool vloaded = Storage.loadWiFi(verify);
    Serial.print("[DEBUG] Verify load: ");
    Serial.println(vloaded && verify.valid ? "OK" : "FAIL");
    Serial.print("[DEBUG] Verify SSID: [");
    Serial.print(verify.ssid);
    Serial.println("]");

    _server.send(200, "text/html", "<h3>Saved. Rebooting...</h3>");
    delay(2000);
    ESP.restart();
  } else {
    _server.send(400, "text/html", "SSID required");
  }
}