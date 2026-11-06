// SafeAir WiFi Manager — STA + SoftAP provisioning
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Config.h"
#include "StorageManager.h"
#include "Logger.h"

enum class WiFiMode { DISCONNECTED, CONNECTING, CONNECTED, AP_MODE };

class WiFiManager {
public:
  bool begin();        // returns true if creds found, false if AP started
  void update();
  WiFiMode mode() const { return _mode; }
  bool isConnected() const { return _mode == WiFiMode::CONNECTED; }
  int rssi() const { return WiFi.RSSI(); }
  String localIP() const { return isConnected() ? WiFi.localIP().toString() : String(); }
  String ssid() const { return isConnected() ? WiFi.SSID() : String(); }
  String apIP() const { return WiFi.softAPIP().toString(); }

  void startProvisioning();
  void stopProvisioning();
  bool isProvisioning() const { return _mode == WiFiMode::AP_MODE; }

private:
  WiFiMode _mode = WiFiMode::DISCONNECTED;
  uint32_t _lastAttempt = 0;
  uint8_t _retryCount = 0;
  bool _provisioning = false;
  uint32_t _provStart = 0;
  bool _serverStarted = false;

  WebServer _server{80};

  bool _connectStored();
  void _enterAPMode();
  void _startProvServer();
  void _handleRoot();
  void _handleSave();
};

extern WiFiManager Net;

#endif