// SafeAir v1.0 — Configuration Header
// Hardware: ESP32-S3-DevKitC-1 | N16R8
// =============================================================================
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Firmware identity
// ---------------------------------------------------------------------------
#define SAFEAIR_FW_VERSION    "1.0.0"
#define SAFEAIR_FW_BUILD      "f7a2c91"
#define SAFEAIR_DEVICE_MODEL  "ESP32-S3-N16R8"

// ---------------------------------------------------------------------------
// GPIO Map (frozen + placeholders)
// ---------------------------------------------------------------------------
// I2C OLED
#define PIN_SDA               8
#define PIN_SCL               9

// Tactile buttons
#define PIN_BTN_PREV          5
#define PIN_BTN_SELECT        6
#define PIN_BTN_NEXT          7

// Status LEDs
#define PIN_LED_GREEN         10
#define PIN_LED_YELLOW        11
#define PIN_LED_RED           12
#define PIN_LED_BLUE          13

// Alarm & suppression
#define PIN_BUZZER            14
#define PIN_PUMP              15   // Placeholder — update after MOSFET/pump check

// Analog inputs
#define PIN_MQ2_A             1    // ADC1_CH0  (GPIO1)
#define PIN_MQ4_A             2    // ADC1_CH1  (GPIO2)
#define PIN_DHT               4    // DHT11 data

// ---------------------------------------------------------------------------
// Timing (milliseconds)
// ---------------------------------------------------------------------------
#define INTERVAL_SENSOR_MS    1000
#define INTERVAL_ALARM_MS     500
#define INTERVAL_DISPLAY_MS   200
#define INTERVAL_BUTTON_MS    50
#define INTERVAL_CLOUD_MS     3000
#define INTERVAL_HEARTBEAT_MS 15000
#define INTERVAL_LOG_FLUSH_MS 5000

#define DEBOUNCE_MS           30
#define LONG_PRESS_MS         800
#define DOUBLE_PRESS_MS       400
#define DISPLAY_TIMEOUT_MS    30000

// ---------------------------------------------------------------------------
// Sensor defaults (will be overridden by NVS after calibration)
// ---------------------------------------------------------------------------
// MQ-2  — LPG + Smoke
#define MQ2_WARN_PPM          300
#define MQ2_DANGER_PPM        800
#define MQ2_MAX_PPM           1000
#define MQ2_BASE_PPM          14

// MQ-4  — Methane
#define MQ4_WARN_PPM          400
#define MQ4_DANGER_PPM        1000
#define MQ4_MAX_PPM           1200
#define MQ4_BASE_PPM          22

// DHT11 — Temperature
#define TEMP_WARN_C           45
#define TEMP_DANGER_C         60
#define TEMP_MAX_C            80
#define TEMP_BASE_C           27

// Humidity (info only, no alarm)
#define HUMID_MAX             100

// Smoke (reserved channel, mapped to MQ-2 for now)
#define SMOKE_WARN_PPM        200
#define SMOKE_DANGER_PPM      500
#define SMOKE_MAX_PPM         800
#define SMOKE_BASE_PPM        9

// ---------------------------------------------------------------------------
// Calibration
// ---------------------------------------------------------------------------
#define CALIBRATION_SAMPLES   50
#define CALIBRATION_INTERVAL  100
#define NVS_KEY_MQ2_RO        "mq2_ro"
#define NVS_KEY_MQ4_RO        "mq4_ro"
#define NVS_KEY_CALIBRATED    "cal_done"

// ---------------------------------------------------------------------------
// Alarm behaviour
// ---------------------------------------------------------------------------
#define BUZZER_FREQ_SAFE      0
#define BUZZER_FREQ_WARN      2    // Hz (slow beep)
#define BUZZER_FREQ_DANGER    8    // Hz (fast / continuous)
#define PUMP_AUTO_ARMED       true

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------
#define OLED_WIDTH            128
#define OLED_HEIGHT           64
#define OLED_ADDR             0x3C
#define OLED_PAGES            6

// ---------------------------------------------------------------------------
// Connectivity
// ---------------------------------------------------------------------------
#define WIFI_MAX_RETRIES      5
#define WIFI_RECONNECT_MS     10000
#define SOFTAP_SSID_PREFIX    "SafeAir-"
#define SOFTAP_PASS           "safestove"
#define SOFTAP_TIMEOUT_MS     300000

// ---------------------------------------------------------------------------
// Firebase (placeholder — replace in production)
// ---------------------------------------------------------------------------
#define FB_HOST               "YOUR_DATABASE_URL/"
#define FB_API_KEY            "YOUR_FIREBASE_API_KEY"
#define FB_DEVICE_PATH        "/devices/"

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------
#define LOG_MAX_EVENTS        60
#define LOG_RING_SIZE         64

// ---------------------------------------------------------------------------
// Enum declarations
// ---------------------------------------------------------------------------
enum class AlarmState { SAFE, WARNING, DANGER };
enum class SystemState { BOOT, RUNNING, CALIBRATING, PROVISIONING, OTA };
enum class ButtonId { NONE, PREV, SELECT, NEXT };
enum class ButtonEvent { NONE, PRESS, RELEASE, LONG_PRESS, DOUBLE_PRESS };
enum class SensorChannel { LPG, METHANE, TEMP, HUMID, SMOKE };

#endif // CONFIG_H