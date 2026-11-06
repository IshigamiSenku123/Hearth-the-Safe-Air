#include "AlarmManager.h"

AlarmManager Alarm;

bool AlarmManager::begin() {
  pinMode(PIN_LED_GREEN,  OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED,    OUTPUT);
  pinMode(PIN_LED_BLUE,   OUTPUT);
  pinMode(PIN_BUZZER,     OUTPUT);

  digitalWrite(PIN_LED_GREEN,  LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);
  digitalWrite(PIN_LED_RED,    LOW);
  digitalWrite(PIN_LED_BLUE,   HIGH);  // Boot indicator
  digitalWrite(PIN_BUZZER,     LOW);
  return true;
}

void AlarmManager::update(const SensorSnapshot& snap, const ThresholdSet& thr) {
  if (_testActive) {
    if (millis() > _testUntil) {
      _testActive = false;
      _state = AlarmState::SAFE;
      SysLog.log(LogLevel::INFO, LogCategory::ALARM, "Test ended", "Alarm test completed");
    } else {
      _state = AlarmState::DANGER;
    }
  } else {
    _evaluate(snap, thr);
  }

  _setLEDs(_state);
  _driveBuzzer(_state);
}

void AlarmManager::_evaluate(const SensorSnapshot& snap, const ThresholdSet& thr) {
  AlarmState newState = AlarmState::SAFE;

  if (snap.lpg.valid      && snap.lpg.value      >= thr.mq2_danger)   newState = AlarmState::DANGER;
  else if (snap.methane.valid && snap.methane.value >= thr.mq4_danger) newState = AlarmState::DANGER;
  else if (snap.temp.valid    && snap.temp.value    >= thr.temp_danger) newState = AlarmState::DANGER;
  else if (snap.smoke.valid   && snap.smoke.value   >= thr.smoke_danger) newState = AlarmState::DANGER;

  if (newState != AlarmState::DANGER) {
    if (snap.lpg.valid      && snap.lpg.value      >= thr.mq2_warn)   newState = AlarmState::WARNING;
    else if (snap.methane.valid && snap.methane.value >= thr.mq4_warn) newState = AlarmState::WARNING;
    else if (snap.temp.valid    && snap.temp.value    >= thr.temp_warn) newState = AlarmState::WARNING;
    else if (snap.smoke.valid   && snap.smoke.value   >= thr.smoke_warn) newState = AlarmState::WARNING;
  }

  if (newState != _state) {
    _state = newState;
    const char* word = (_state == AlarmState::SAFE) ? "SAFE" :
                       (_state == AlarmState::WARNING) ? "WARNING" : "DANGER";
    SysLog.log(LogLevel::INFO, LogCategory::ALARM,
      "State change", "Alarm state -> %s", word);
  }
}

void AlarmManager::_setLEDs(AlarmState s) {
  digitalWrite(PIN_LED_GREEN,  s == AlarmState::SAFE    ? HIGH : LOW);
  digitalWrite(PIN_LED_YELLOW, s == AlarmState::WARNING ? HIGH : LOW);
  digitalWrite(PIN_LED_RED,    s == AlarmState::DANGER  ? HIGH : LOW);
  digitalWrite(PIN_LED_BLUE,   LOW);  // Blue only used during boot
}

void AlarmManager::_driveBuzzer(AlarmState s) {
  if (_muted) {
    digitalWrite(PIN_BUZZER, LOW);
    _buzzerOn = false;
    return;
  }

  uint32_t now = millis();

  if (s == AlarmState::SAFE) {
    digitalWrite(PIN_BUZZER, LOW);
    _buzzerOn = false;
  }
  else if (s == AlarmState::WARNING) {
    // Slow beep: 0.5s ON, 0.5s OFF
    if (now - _lastBuzzerToggle >= 500) {
      _lastBuzzerToggle = now;
      _buzzerOn = !_buzzerOn;
      digitalWrite(PIN_BUZZER, _buzzerOn ? HIGH : LOW);
    }
  }
  else if (s == AlarmState::DANGER) {
    // Continuous / fast beep: 120ms ON, 80ms OFF
    if (now - _lastBuzzerToggle >= (_buzzerOn ? 120 : 80)) {
      _lastBuzzerToggle = now;
      _buzzerOn = !_buzzerOn;
      digitalWrite(PIN_BUZZER, _buzzerOn ? HIGH : LOW);
    }
  }
}

void AlarmManager::mute(bool m) {
  if (_muted != m) {
    _muted = m;
    SysLog.log(LogLevel::INFO, LogCategory::USER,
      _muted ? "Muted" : "Unmuted",
      "Buzzer %s", _muted ? "silenced" : "active");
  }
}

void AlarmManager::testAlarm(uint32_t durationMs) {
  _testActive = true;
  _testUntil = millis() + durationMs;
  _muted = false;
  SysLog.log(LogLevel::INFO, LogCategory::ALARM, "Test started",
    "Manual alarm test for %u ms", durationMs);
}

void AlarmManager::stopTest() {
  _testActive = false;
}