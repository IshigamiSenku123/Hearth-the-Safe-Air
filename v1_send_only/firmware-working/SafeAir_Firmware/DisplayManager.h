// SafeAir Display Manager — SSD1306 OLED UI / pages
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Config.h"
#include "SensorManager.h"
#include "AlarmManager.h"
#include "PumpManager.h"
#include "StorageManager.h"

class DisplayManager {
public:
  bool begin();
  void update(const SensorSnapshot& snap, AlarmState alarm, const DeviceMeta& meta);
  void nextPage();
  void prevPage();
  void setPage(uint8_t p);
  uint8_t page() const { return _page; }
  void setContrast(uint8_t pct);  // 10-100

private:
  Adafruit_SSD1306 _oled{OLED_WIDTH, OLED_HEIGHT, &Wire, -1};
  uint8_t _page = 0;
  uint8_t _contrast = 100;
  uint32_t _lastActivity = 0;
  bool _dimmed = false;

  void _drawPage0_Status(AlarmState alarm, const SensorSnapshot& snap);
  void _drawPage1_TempHum(const SensorSnapshot& snap);
  void _drawPage2_LPG(const SensorSnapshot& snap);
  void _drawPage3_Methane(const SensorSnapshot& snap);
  void _drawPage4_Smoke(const SensorSnapshot& snap);
  void _drawPage5_DeviceInfo(const DeviceMeta& meta, AlarmState alarm);
  void _drawBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, float pct);
  void _drawHeader(const char* title, uint8_t pageNum);
};

extern DisplayManager Display;

#endif
 