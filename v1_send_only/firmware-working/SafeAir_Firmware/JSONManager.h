// SafeAir JSON Manager — serialization / deserialization
#ifndef JSON_MANAGER_H
#define JSON_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "SensorManager.h"
#include "Logger.h"

class JSONManager {
public:
  // Device → Cloud payloads
  static size_t buildTelemetry(char* buf, size_t len,
                               const SensorSnapshot& snap,
                               AlarmState alarm,
                               const char* deviceId,
                               uint32_t uptimeSec,
                               int rssi);

  static size_t buildHeartbeat(char* buf, size_t len,
                               const char* deviceId,
                               const char* fwVer,
                               const char* ip,
                               const char* ssid,
                               uint32_t uptimeSec);

  static size_t buildAck(char* buf, size_t len,
                         const char* cmdId,
                         bool success,
                         const char* message);

  static size_t buildLogBatch(char* buf, size_t len,
                              const Logger& logger,
                              size_t maxEvents = 20);

  // Cloud → Device parsing
  struct Command {
    char id[24];
    char type[24];
    char payload[128];
    bool valid = false;
  };
  static Command parseCommand(const char* json);
};

#endif