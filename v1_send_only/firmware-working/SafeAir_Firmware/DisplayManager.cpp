#include "DisplayManager.h"
#include <WiFi.h>

DisplayManager Display;

bool DisplayManager::begin() {
  Wire.setPins(PIN_SDA, PIN_SCL);
  if (!_oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[DISPLAY] SSD1306 init failed");
    return false;
  }
  _oled.clearDisplay();
  _oled.setTextColor(SSD1306_WHITE);
  _oled.display();
  _lastActivity = millis();
  return true;
}

void DisplayManager::setContrast(uint8_t pct) {
  _contrast = constrain(pct, 10, 100);
  _oled.ssd1306_command(SSD1306_SETCONTRAST);
  _oled.ssd1306_command(_contrast * 255 / 100);
}

void DisplayManager::nextPage() {
  _page = (_page + 1) % OLED_PAGES;
  _lastActivity = millis();
  _dimmed = false;
}

void DisplayManager::prevPage() {
  _page = (_page + OLED_PAGES - 1) % OLED_PAGES;
  _lastActivity = millis();
  _dimmed = false;
}

void DisplayManager::setPage(uint8_t p) {
  if (p < OLED_PAGES) _page = p;
  _lastActivity = millis();
  _dimmed = false;
}

void DisplayManager::update(const SensorSnapshot& snap, AlarmState alarm, const DeviceMeta& meta) {
  if (!_dimmed && (millis() - _lastActivity > DISPLAY_TIMEOUT_MS)) {
    setContrast(20);
    _dimmed = true;
  }

  _oled.clearDisplay();

  if (alarm == AlarmState::DANGER && (millis() % 1000 < 500)) {
    _oled.fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT, SSD1306_WHITE);
    _oled.setTextColor(SSD1306_BLACK);
    _oled.setCursor(20, 28);
    _oled.setTextSize(2);
    _oled.print("! ALARM !");
    _oled.setCursor(14, 50);
    _oled.setTextSize(1);
    _oled.print("DANGER DETECTED");
    _oled.display();
    return;
  }

  _oled.setTextColor(SSD1306_WHITE);

  switch (_page) {
    case 0: _drawPage0_Status(alarm, snap); break;
    case 1: _drawPage1_TempHum(snap); break;
    case 2: _drawPage2_LPG(snap); break;
    case 3: _drawPage3_Methane(snap); break;
    case 4: _drawPage4_Smoke(snap); break;
    case 5: _drawPage5_DeviceInfo(meta, alarm); break;
  }
  _oled.display();
}

void DisplayManager::_drawHeader(const char* title, uint8_t pageNum) {
  _oled.setTextSize(1);
  _oled.setCursor(0, 0);
  _oled.print(title);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d/%d", pageNum + 1, OLED_PAGES);
  int16_t tw = strlen(buf) * 6;
  _oled.setCursor(OLED_WIDTH - tw, 0);
  _oled.print(buf);
  _oled.drawLine(0, 9, OLED_WIDTH, 9, SSD1306_WHITE);
}

void DisplayManager::_drawBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, float pct) {
  pct = constrain(pct, 0.0f, 1.0f);
  _oled.drawRect(x, y, w, h, SSD1306_WHITE);
  uint8_t fill = (uint8_t)(pct * (w - 2));
  if (fill > 0) _oled.fillRect(x + 1, y + 1, fill, h - 2, SSD1306_WHITE);
}

void DisplayManager::_drawPage0_Status(AlarmState alarm, const SensorSnapshot& snap) {
  _oled.setTextSize(1);
  _oled.setCursor(0, 0);
  _oled.print("SAFEAIR");
  _oled.setCursor(90, 0);
  _oled.print(WiFi.isConnected() ? "WiFi" : "--");
  _oled.drawLine(0, 9, OLED_WIDTH, 9, SSD1306_WHITE);

  const char* word = (alarm == AlarmState::SAFE) ? "SAFE" :
                     (alarm == AlarmState::WARNING) ? "WARN" : "DANGER";
  _oled.setTextSize(2);
  _oled.setCursor(30, 18);
  _oled.print(word);

  _oled.setTextSize(1);
  _oled.setCursor(20, 40);
  _oled.printf("Sys %s", Alarm.isMuted() ? "muted" : "armed");

  _oled.setCursor(0, 54);
  _oled.printf("G:%.0f S:%.0f T:%.0f", snap.methane.value, snap.smoke.value, snap.temp.value);
}

void DisplayManager::_drawPage1_TempHum(const SensorSnapshot& snap) {
  _drawHeader("TEMP / HUMID", 1);
  _oled.setTextSize(2);
  _oled.setCursor(0, 14);
  if (snap.temp.valid) {
    _oled.printf("%.1f C", snap.temp.value);
  } else {
    _oled.print("--");
  }
  _oled.setTextSize(1);
  _oled.setCursor(0, 36);
  if (snap.humid.valid) {
    _oled.printf("Humidity: %.1f %%", snap.humid.value);
  } else {
    _oled.print("Humidity: --");
  }
  if (snap.temp.valid) {
    float pct = constrain(snap.temp.value / TEMP_MAX_C, 0.0f, 1.0f);
    _drawBar(0, 50, 120, 10, pct);
  }
}

void DisplayManager::_drawPage2_LPG(const SensorSnapshot& snap) {
  _drawHeader("LPG (MQ-2)", 2);
  _oled.setTextSize(2);
  _oled.setCursor(0, 14);
  _oled.printf("%.0f", snap.lpg.value);
  _oled.setTextSize(1);
  _oled.setCursor(70, 22);
  _oled.print("ppm");
  float pct = constrain(snap.lpg.value / MQ2_MAX_PPM, 0.0f, 1.0f);
  _drawBar(0, 40, 120, 10, pct);
  _oled.setCursor(0, 54);
  _oled.printf("Avg:%.0f Pk:%.0f", snap.lpg.avg40, snap.lpg.peak40);
}

void DisplayManager::_drawPage3_Methane(const SensorSnapshot& snap) {
  _drawHeader("METHANE (MQ-4)", 3);
  _oled.setTextSize(2);
  _oled.setCursor(0, 14);
  _oled.printf("%.0f", snap.methane.value);
  _oled.setTextSize(1);
  _oled.setCursor(70, 22);
  _oled.print("ppm");
  float pct = constrain(snap.methane.value / MQ4_MAX_PPM, 0.0f, 1.0f);
  _drawBar(0, 40, 120, 10, pct);
  _oled.setCursor(0, 54);
  _oled.printf("Avg:%.0f Pk:%.0f", snap.methane.avg40, snap.methane.peak40);
}

void DisplayManager::_drawPage4_Smoke(const SensorSnapshot& snap) {
  _drawHeader("SMOKE", 4);
  _oled.setTextSize(2);
  _oled.setCursor(0, 14);
  _oled.printf("%.0f", snap.smoke.value);
  _oled.setTextSize(1);
  _oled.setCursor(70, 22);
  _oled.print("ppm");
  float pct = constrain(snap.smoke.value / SMOKE_MAX_PPM, 0.0f, 1.0f);
  _drawBar(0, 40, 120, 10, pct);
  _oled.setCursor(0, 54);
  _oled.print("Reserved: future sensor");
}

void DisplayManager::_drawPage5_DeviceInfo(const DeviceMeta& meta, AlarmState alarm) {
  _drawHeader("DEVICE INFO", 5);
  _oled.setTextSize(1);
  _oled.setCursor(0, 12);
  _oled.printf("%s", meta.deviceName);
  _oled.setCursor(0, 22);
  _oled.printf("ESP32-S3 N16R8");
  _oled.setCursor(0, 32);
  _oled.printf("FW v%s", SAFEAIR_FW_VERSION);
  _oled.setCursor(0, 42);
  _oled.printf("Boot #%lu", meta.bootCount);
  _oled.setCursor(0, 54);
  const char* st = (alarm == AlarmState::SAFE) ? "OK" :
                   (alarm == AlarmState::WARNING) ? "CAUTION" : "ALARM";
  _oled.printf("Status: %s", st);
}