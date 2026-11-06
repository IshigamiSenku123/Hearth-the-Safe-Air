#include "WiFiManager.h"

WiFiManager Net;

bool WiFiManager::begin() {
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);
  _connectStored();
  return true;
}

void WiFiManager::update() {
  if (_provisioning) {
    if (millis() - _provStart > SOFTAP_TIMEOUT_MS) {
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

void WiFiManager::_connectStored() {
  WiFiCredentials cred;
  if (Storage.loadWiFi(cred) && cred.valid) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(cred.ssid, cred.password);
    _mode = WiFiMode::CONNECTING;
    _lastAttempt = millis();
    _retryCount = 0;
    SysLog.log(LogLevel::INFO, LogCategory::NETWORK,
      "WiFi connecting", "SSID=%s", cred.ssid);
  } else {
    SysLog.log(LogLevel::INFO, LogCategory::NETWORK,
      "No WiFi creds", "Starting provisioning mode");
    _enterAPMode();
  }
}

void WiFiManager::_enterAPMode() {
  WiFi.mode(WIFI_AP_STA);
  String apName = String(SOFTAP_SSID_PREFIX) + String((uint32_t)ESP.getEfuseMac(), HEX);
  WiFi.softAP(apName.c_str(), SOFTAP_PASS);
  _mode = WiFiMode::AP_MODE;
  _provisioning = true;
  _provStart = millis();
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
    _connectStored();
  }
}