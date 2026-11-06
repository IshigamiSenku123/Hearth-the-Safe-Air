// SafeAir WiFi Manager — STA + SoftAP provisioning
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "StorageManager.h"
#include "Logger.h"

enum class WiFiMode { DISCONNECTED, CONNECTING, CONNECTED, AP_MODE };

class WiFiManager {
public:
  bool begin();
  void update();  // call every INTERVAL_CLOUD_MS
  WiFiMode mode() const { return _mode; }
  bool isConnected() const { return _mode == WiFiMode::CONNECTED; }
  int rssi() const { return WiFi.RSSI(); }
  String localIP() const { return isConnected() ? WiFi.localIP().toString() : String(); }
  String ssid() const { return isConnected() ? WiFi.SSID() : String(); }

  void startProvisioning();
  void stopProvisioning();
  bool isProvisioning() const { return _mode == WiFiMode::AP_MODE; }

private:
  WiFiMode _mode = WiFiMode::DISCONNECTED;
  uint32_t _lastAttempt = 0;
  uint8_t _retryCount = 0;
  bool _provisioning = false;
  uint32_t _provStart = 0;

  void _connectStored();
  void _enterAPMode();
};

extern WiFiManager Net;

#endif