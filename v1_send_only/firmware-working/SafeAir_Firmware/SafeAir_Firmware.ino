/*
  SafeAir v1.0 — ESP32-S3 N16R8
  Architecture: Application + System + Hardware layers
  FreeRTOS tasks, no blocking delays in runtime
*/

#include "Config.h"
#include "StorageManager.h"
#include "Logger.h"
#include "SensorManager.h"
#include "AlarmManager.h"
#include "PumpManager.h"
#include "ButtonManager.h"
#include "DisplayManager.h"
#include "WiFiManager.h"
#include "FirebaseManager.h"
#include "JSONManager.h"
#include "DeviceManager.h"

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
ThresholdSet thresholds;
volatile bool g_commandReady = false;

// ---------------------------------------------------------------------------
// FreeRTOS task prototypes
// ---------------------------------------------------------------------------
void TaskSensors(void* pv);
void TaskAlarm(void* pv);
void TaskDisplay(void* pv);
void TaskButtons(void* pv);
void TaskCloud(void* pv);

void setup() {
  Serial.begin(115200);
  delay(500);  // Only delay in boot path
  Serial.println("\n[BOOT] SafeAir " SAFEAIR_FW_VERSION);

  // Hardware layer init
  Storage.begin();
  SysLog.begin();
  Sensors.begin();
  Alarm.begin();
  Pump.begin();
  Buttons.begin();
  Display.begin();

  // Load thresholds from NVS (or defaults)
  Storage.loadThresholds(thresholds);

  // System layer init
  Net.begin();

  // Firebase init (only if WiFi already up; otherwise retries in TaskCloud)
  if (Net.isConnected()) {
    Cloud.begin(FB_HOST, FB_API_KEY, Device.meta().deviceName);
  }

  Device.begin();

  // Boot LED sequence
  digitalWrite(PIN_LED_BLUE, LOW);
  digitalWrite(PIN_LED_GREEN, HIGH);

  // -------------------------------------------------------------------------
  // Create FreeRTOS tasks
  // -------------------------------------------------------------------------
  xTaskCreatePinnedToCore(TaskSensors, "Sensors", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskAlarm,   "Alarm",   4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskDisplay, "Display", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskButtons, "Buttons", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskCloud,   "Cloud",   8192, NULL, 1, NULL, 0);

  Serial.println("[BOOT] Tasks created. System running.");
}

void loop() {
  // Main loop is idle — all work is in tasks.
  // We use this for watchdog feed and low-priority house-keeping.
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// ===========================================================================
// Task: Sensor acquisition (1 Hz)
// ===========================================================================
void TaskSensors(void* pv) {
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    Sensors.update();
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(INTERVAL_SENSOR_MS));
  }
}

// ===========================================================================
// Task: Alarm evaluation + actuators (2 Hz)
// ===========================================================================
void TaskAlarm(void* pv) {
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    const SensorSnapshot& snap = Sensors.read();
    Alarm.update(snap, thresholds);
    Pump.update(Alarm.state());
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(INTERVAL_ALARM_MS));
  }
}

// ===========================================================================
// Task: OLED UI (5 Hz)
// ===========================================================================
void TaskDisplay(void* pv) {
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    Display.update(Sensors.read(), Alarm.state(), Device.meta());
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(INTERVAL_DISPLAY_MS));
  }
}

// ===========================================================================
// Task: Button polling + UI navigation (20 Hz)
// ===========================================================================
void TaskButtons(void* pv) {
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    Buttons.update();
    while (Buttons.hasEvent()) {
      ButtonEventData ev = Buttons.getEvent();
      if (ev.evt == ButtonEvent::PRESS) {
        if (ev.id == ButtonId::PREV)  Display.prevPage();
        if (ev.id == ButtonId::NEXT)  Display.nextPage();
        if (ev.id == ButtonId::SELECT) {
          // Single press: nothing (or confirm)
          // Long press handled separately
        }
      }
      else if (ev.evt == ButtonEvent::LONG_PRESS && ev.id == ButtonId::SELECT) {
        Alarm.mute(!Alarm.isMuted());
      }
      else if (ev.evt == ButtonEvent::DOUBLE_PRESS && ev.id == ButtonId::SELECT) {
        Alarm.mute(true);  // Quick mute
      }
    }
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(INTERVAL_BUTTON_MS));
  }
}

// ===========================================================================
// Task: WiFi + Firebase + Command dispatch (0.33 Hz, but checks faster)
// ===========================================================================
void TaskCloud(void* pv) {
  TickType_t lastWake = xTaskGetTickCount();
  static uint32_t lastNetCheck = 0;
  for (;;) {
    uint32_t now = millis();

    // WiFi state machine
    if (now - lastNetCheck >= INTERVAL_CLOUD_MS) {
      lastNetCheck = now;
      Net.update();
      Device.update();

      // Lazy-init Firebase once WiFi is up
      if (Net.isConnected() && !Cloud.isReady()) {
        Cloud.begin(FB_HOST, FB_API_KEY, Device.meta().deviceName);
      }
      Cloud.update();
    }

    // Command dispatch
    if (Cloud.hasCommand()) {
      JSONManager::Command cmd = Cloud.getCommand();
      Device.onCommand(cmd);
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(INTERVAL_CLOUD_MS / 3));
  }
}