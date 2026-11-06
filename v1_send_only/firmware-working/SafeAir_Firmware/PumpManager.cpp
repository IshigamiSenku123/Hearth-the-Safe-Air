#include "PumpManager.h"

PumpManager Pump;

bool PumpManager::begin() {
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW);
  return true;
}

void PumpManager::arm(bool armed) {
  if (_armed != armed) {
    _armed = armed;
    SysLog.log(LogLevel::INFO, LogCategory::SYSTEM,
      armed ? "Pump armed" : "Pump disarmed",
      "Auto-suppression %s", armed ? "enabled" : "disabled");
  }
}

void PumpManager::trigger() {
  if (!_running) {
    _running = true;
    _runStart = millis();
    digitalWrite(PIN_PUMP, HIGH);
    SysLog.log(LogLevel::CRITICAL, LogCategory::ALARM,
      "Pump ON", "Water suppression activated");
  }
}

void PumpManager::stop() {
  if (_running) {
    _running = false;
    digitalWrite(PIN_PUMP, LOW);
    SysLog.log(LogLevel::INFO, LogCategory::ALARM,
      "Pump OFF", "Water suppression stopped");
  }
}

void PumpManager::update(AlarmState alarmState) {
  if (_armed && alarmState == AlarmState::DANGER) {
    trigger();
  }
  if (_running && (millis() - _runStart > MAX_RUN_MS)) {
    SysLog.log(LogLevel::WARNING, LogCategory::ALARM,
      "Pump timeout", "Auto-cutoff after %u ms", MAX_RUN_MS);
    stop();
  }
  if (_running && alarmState == AlarmState::SAFE) {
    stop();
  }
}