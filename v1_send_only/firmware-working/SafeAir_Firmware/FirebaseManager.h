// SafeAir Firebase Manager — RTDB sync, command listener
#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <FirebaseESP32.h>
#include "Config.h"
#include "WiFiManager.h"
#include "JSONManager.h"
#include "Logger.h"

class FirebaseManager {
public:
  bool begin(const char* host, const char* apiKey, const char* deviceId);
  void update();
  bool isReady() const { return _ready; }

  void pushTelemetry(const char* json);
  void pushHeartbeat(const char* json);
  void sendAck(const char* json);
  void uploadLogs(const char* json);

  bool hasCommand() const { return _cmdPending; }
  JSONManager::Command getCommand();

private:
  FirebaseData _fbData;
  FirebaseAuth _auth;
  FirebaseConfig _config;
  bool _ready = false;
  bool _cmdPending = false;
  JSONManager::Command _lastCmd{};
  char _devicePath[64];
  char _cmdPath[80];
  uint32_t _lastStreamCheck = 0;

  static void _streamCallback(StreamData data);
  static void _streamTimeoutCallback(bool timeout);
};

extern FirebaseManager Cloud;

#endif