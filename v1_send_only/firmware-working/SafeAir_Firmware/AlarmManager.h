// SafeAir Alarm Manager — state machine, LEDs, buzzer
#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <Arduino.h>
#include "Config.h"
#include "SensorManager.h"
#include "StorageManager.h"
#include "Logger.h"

class AlarmManager {
public:
  bool begin();
  void update(const SensorSnapshot& snap, const ThresholdSet& thr);
  AlarmState state() const { return _state; }
  void mute(bool m);
  bool isMuted() const { return _muted; }
  void testAlarm(uint32_t durationMs = 5000);
  void stopTest();
  bool isTesting() const { return _testActive; }

private:
  AlarmState _state = AlarmState::SAFE;
  bool _muted = false;
  bool _testActive = false;
  uint32_t _testUntil = 0;
  uint32_t _lastBuzzerToggle = 0;
  bool _buzzerOn = false;

  void _setLEDs(AlarmState s);
  void _driveBuzzer(AlarmState s);
  void _evaluate(const SensorSnapshot& snap, const ThresholdSet& thr);
};

extern AlarmManager Alarm;

#endif