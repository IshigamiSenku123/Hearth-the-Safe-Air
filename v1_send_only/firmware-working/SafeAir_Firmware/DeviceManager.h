// SafeAir Device Manager — identity, uptime, command dispatch
#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <Arduino.h>
#include "Config.h"
#include "StorageManager.h"
#include "SensorManager.h"
#include "AlarmManager.h"
#include "PumpManager.h"
#include "Logger.h"
#include "JSONManager.h"

class DeviceManager {
public:
  bool begin();
  void update();  // heartbeat, cloud sync decisions

  const char* id() const { return _meta.deviceName; }
  const DeviceMeta& meta() const { return _meta; }
  uint32_t uptime() const { return millis() / 1000; }

  void onCommand(const JSONManager::Command& cmd);
  void factoryReset();

private:
  DeviceMeta _meta{};
  uint32_t _lastTelem = 0;
  uint32_t _lastHb = 0;
  uint32_t _startMs = 0;
  char _jsonBuf[1024];

  void _sendTelemetry();
  void _sendHeartbeat();
};

extern DeviceManager Device;
#endif