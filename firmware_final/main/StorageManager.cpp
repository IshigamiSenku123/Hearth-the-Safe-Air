#include "StorageManager.h"

StorageManager Storage;

bool StorageManager::begin() {
  return prefs.begin(NAMESPACE, false);
}

void StorageManager::end() {
  prefs.end();
}

bool StorageManager::loadThresholds(ThresholdSet& t) {
  t.mq2_warn   = prefs.getUShort("mq2_w", MQ2_WARN_PPM);
  t.mq2_danger = prefs.getUShort("mq2_d", MQ2_DANGER_PPM);
  t.mq4_warn   = prefs.getUShort("mq4_w", MQ4_WARN_PPM);
  t.mq4_danger = prefs.getUShort("mq4_d", MQ4_DANGER_PPM);
  t.temp_warn  = prefs.getUShort("tmp_w", TEMP_WARN_C);
  t.temp_danger= prefs.getUShort("tmp_d", TEMP_DANGER_C);
  t.smoke_warn = prefs.getUShort("smk_w", SMOKE_WARN_PPM);
  t.smoke_danger=prefs.getUShort("smk_d", SMOKE_DANGER_PPM);
  return true;
}

bool StorageManager::saveThresholds(const ThresholdSet& t) {
  prefs.putUShort("mq2_w", t.mq2_warn);
  prefs.putUShort("mq2_d", t.mq2_danger);
  prefs.putUShort("mq4_w", t.mq4_warn);
  prefs.putUShort("mq4_d", t.mq4_danger);
  prefs.putUShort("tmp_w", t.temp_warn);
  prefs.putUShort("tmp_d", t.temp_danger);
  prefs.putUShort("smk_w", t.smoke_warn);
  prefs.putUShort("smk_d", t.smoke_danger);
  return true;
}

bool StorageManager::loadCalibration(float& mq2Ro, float& mq4Ro, bool& done) {
  mq2Ro = prefs.getFloat(NVS_KEY_MQ2_RO, 10.0f);
  mq4Ro = prefs.getFloat(NVS_KEY_MQ4_RO, 10.0f);
  done  = prefs.getBool(NVS_KEY_CALIBRATED, false);
  return true;
}

bool StorageManager::saveCalibration(float mq2Ro, float mq4Ro) {
  prefs.putFloat(NVS_KEY_MQ2_RO, mq2Ro);
  prefs.putFloat(NVS_KEY_MQ4_RO, mq4Ro);
  prefs.putBool(NVS_KEY_CALIBRATED, true);
  return true;
}

bool StorageManager::loadWiFi(WiFiCredentials& cred) {
  cred.valid = prefs.getBool("wifi_valid", false);
  if (cred.valid) {
    strlcpy(cred.ssid,     prefs.getString("wifi_ssid", "").c_str(), sizeof(cred.ssid));
    strlcpy(cred.password, prefs.getString("wifi_pass", "").c_str(), sizeof(cred.password));
  }
  return cred.valid;
}

bool StorageManager::saveWiFi(const WiFiCredentials& cred) {
  prefs.putString("wifi_ssid", cred.ssid);
  prefs.putString("wifi_pass", cred.password);
  prefs.putBool("wifi_valid", true);
  return true;
}

bool StorageManager::clearWiFi() {
  prefs.putBool("wifi_valid", false);
  prefs.remove("wifi_ssid");
  prefs.remove("wifi_pass");
  return true;
}

bool StorageManager::loadMeta(DeviceMeta& m) {
  strlcpy(m.deviceName, prefs.getString("dev_name", "SafeAir").c_str(), sizeof(m.deviceName));
  strlcpy(m.roomName,   prefs.getString("room_name", "Kitchen").c_str(), sizeof(m.roomName));
  m.bootCount = prefs.getUInt("boot_cnt", 0);
  return true;
}

bool StorageManager::saveMeta(const DeviceMeta& m) {
  prefs.putString("dev_name", m.deviceName);
  prefs.putString("room_name", m.roomName);
  prefs.putUInt("boot_cnt", m.bootCount);
  return true;
}

uint32_t StorageManager::incrementBootCount() {
  uint32_t cnt = prefs.getUInt("boot_cnt", 0) + 1;
  prefs.putUInt("boot_cnt", cnt);
  return cnt;
}

bool StorageManager::putString(const char* key, const char* value) {
  return prefs.putString(key, value) > 0;
}

String StorageManager::getString(const char* key, const char* def) {
  return prefs.getString(key, def);
}