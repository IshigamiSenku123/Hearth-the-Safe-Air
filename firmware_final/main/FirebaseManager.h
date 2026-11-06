#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "Config.h"
#include "JSONManager.h"
#include "Logger.h"
#include "WiFiManager.h"

class FirebaseManager {
public:
  bool begin(const char* host, const char* apiKey, const char* deviceId);
  void update();
  bool isReady() const { return _ready; }
  bool hasCommand() const { return _cmdPending; }
  JSONManager::Command getCommand();

  void pushTelemetry(const char* json);
  void pushHeartbeat(const char* json);
  void sendAck(const char* json);
  void uploadLogs(const char* json);
  
  void clearCommands();  // NEW: delete processed commands from RTDB

private:
  bool _ready = false;
  bool _cmdPending = false;
  char _devicePath[64] = {0};
  char _cmdPath[72] = {0};
  char _host[128] = {0};
  JSONManager::Command _lastCmd;
  
  WiFiClientSecure _client;
  unsigned long _lastCmdPoll = 0;
  static const unsigned long CMD_POLL_INTERVAL = 1000;

  bool _httpPut(const char* path, const char* json);
  bool _httpPost(const char* path, const char* json);
  bool _httpDelete(const char* path);  // NEW
  String _httpGet(const char* path);
  String _buildUrl(const char* path);
};

extern FirebaseManager Cloud;

#endif