// SafeAir Storage Manager — NVS abstraction
#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

struct ThresholdSet {
  uint16_t mq2_warn, mq2_danger;
  uint16_t mq4_warn, mq4_danger;
  uint16_t temp_warn, temp_danger;
  uint16_t smoke_warn, smoke_danger;
};

struct WiFiCredentials {
  char ssid[32];
  char password[64];
  bool valid;
};

struct DeviceMeta {
  char deviceName[32];
  char roomName[32];
  uint32_t bootCount;
};

class StorageManager {
public:
  bool begin();
  void end();

  // Thresholds
  bool loadThresholds(ThresholdSet& t);
  bool saveThresholds(const ThresholdSet& t);

  // Calibration
  bool loadCalibration(float& mq2Ro, float& mq4Ro, bool& done);
  bool saveCalibration(float mq2Ro, float mq4Ro);

  // WiFi
  bool loadWiFi(WiFiCredentials& cred);
  bool saveWiFi(const WiFiCredentials& cred);
  bool clearWiFi();

  // Meta
  bool loadMeta(DeviceMeta& m);
  bool saveMeta(const DeviceMeta& m);
  uint32_t incrementBootCount();

  // Generic key-value
  bool putString(const char* key, const char* value);
  String getString(const char* key, const char* def);

private:
  Preferences prefs;
  static constexpr const char* NAMESPACE = "safeair";
};

extern StorageManager Storage;

#endif