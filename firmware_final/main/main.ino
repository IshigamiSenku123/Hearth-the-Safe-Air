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

// ===========================================================================
// setup()
// ===========================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[BOOT] SafeAir " SAFEAIR_FW_VERSION);

  // =====================================================================
  // PHASE 1: HARDWARE — always first. Safety system must work without WiFi.
  // =====================================================================
  Storage.begin();
  SysLog.begin();
  Sensors.begin();
  Alarm.begin();
  Pump.begin();
  Buttons.begin();
  Display.begin();

  Storage.loadThresholds(thresholds);

  digitalWrite(PIN_LED_BLUE, LOW);
  digitalWrite(PIN_LED_GREEN, HIGH);

  // =====================================================================
  // PHASE 2: SAFETY TASKS — start immediately, regardless of network state.
  // =====================================================================
  xTaskCreatePinnedToCore(TaskSensors, "Sensors", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskAlarm,   "Alarm",   4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskDisplay, "Display", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskButtons, "Buttons", 2048, NULL, 2, NULL, 1);

  Serial.println("[BOOT] Safety tasks running. Sensors active.");

  // =====================================================================
  // PHASE 3: WIFI — block up to 10 s for connection, then decide path
  // =====================================================================
  bool wifiOk = Net.begin();  // true = creds found, false = AP started

  Serial.print("[BOOT] Waiting for WiFi");
  int wifiWait = 0;
  while (!Net.isConnected() && wifiWait < 20) {
    Net.update();           // let state machine run
    delay(500);
    Serial.print(".");
    wifiWait++;
  }
  Serial.println();

  if (Net.isConnected()) {
    // WiFi is UP — initialize Firebase NOW
    Serial.println("[WIFI] Connected. SSID: " + Net.ssid());
    Serial.println("[WIFI] IP: " + Net.localIP());

    Device.begin();  // ← FIRST: initializes meta including deviceName
    Cloud.begin(FB_HOST, FB_API_KEY, Device.meta().deviceName);  // ← SECOND
    Serial.println("[BOOT] Firebase initialized.");

    xTaskCreatePinnedToCore(TaskCloud, "Cloud", 8192, NULL, 1, NULL, 0);
    Serial.println("[BOOT] Cloud task started. Full system online.");

  } else if (Net.isProvisioning()) {
    // No creds — AP mode is already active
    Device.begin();
    Serial.println("[WIFI] AP mode active. SSID: SafeAir-XXXX");
    Serial.println("[WIFI] Pass: safeair123 | Open http://192.168.4.1");
    Serial.println("[BOOT] Running in LOCAL-ONLY safety mode.");

  } else {
    // Creds exist but connection failed — force AP mode so user can re-enter
    Serial.println("[WIFI] WiFi failed. Forcing provisioning mode.");
    Net.startProvisioning();
    Device.begin();
    Serial.println("[WIFI] AP mode active. Re-enter credentials.");
  }

  Serial.println("[BOOT] Setup complete.");
}

// ===========================================================================
// loop()
// ===========================================================================
void loop() {
  // Service the provisioning web server when in AP mode
  Net.update();
  vTaskDelay(pdMS_TO_TICKS(50));
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
    if (Net.isProvisioning()) {
      // Optional: show provisioning banner
      // Display.showProvisioningBanner(Net.apIP());
      Display.update(Sensors.read(), Alarm.state(), Device.meta());
    } else {
      Display.update(Sensors.read(), Alarm.state(), Device.meta());
    }
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
// Task: WiFi + Firebase + Command dispatch
// ===========================================================================
void TaskCloud(void* pv) {
  TickType_t lastWake = xTaskGetTickCount();
  static uint32_t lastNetCheck = 0;
  static uint32_t lastTelemetry = 0;
  static uint32_t lastHeartbeat = 0;
  
  for (;;) {
    uint32_t now = millis();

    if (now - lastNetCheck >= INTERVAL_CLOUD_MS) {
      lastNetCheck = now;
      Net.update();
      if (Net.isConnected() && !Cloud.isReady()) {
        Cloud.begin(FB_HOST, FB_API_KEY, Device.meta().deviceName);
      }
    }

    if (Cloud.isReady()) {
      Cloud.update();
    }

    // Command dispatch
    if (Cloud.hasCommand()) {
      JSONManager::Command cmd = Cloud.getCommand();
      Device.onCommand(cmd);
      Cloud.clearCommands();  // NEW: prevent re-processing
    }

    Device.update();

    if (Cloud.isReady() && (now - lastTelemetry >= INTERVAL_CLOUD_MS)) {
      lastTelemetry = now;
      char json[512];
      size_t n = JSONManager::buildTelemetry(json, sizeof(json),
        Sensors.read(), Alarm.state(), Device.meta().deviceName,
        now / 1000, Net.isConnected() ? Net.rssi() : 0);
      if (n > 0 && n < sizeof(json)) {
        Cloud.pushTelemetry(json);
      }
    }

    if (Cloud.isReady() && (now - lastHeartbeat >= INTERVAL_HEARTBEAT_MS)) {
      lastHeartbeat = now;
      char json[256];
      size_t n = JSONManager::buildHeartbeat(json, sizeof(json),
        Device.meta().deviceName, SAFEAIR_FW_VERSION,
        Net.isConnected() ? Net.localIP().c_str() : "0.0.0.0",
        Net.isConnected() ? Net.ssid().c_str() : "none", now / 1000);
      if (n > 0 && n < sizeof(json)) {
        Cloud.pushHeartbeat(json);
      }
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(100));
  }
}