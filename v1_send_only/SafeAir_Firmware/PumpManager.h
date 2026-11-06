// SafeAir Pump Manager — water suppression via MOSFET
#ifndef PUMP_MANAGER_H
#define PUMP_MANAGER_H

#include <Arduino.h>
#include "Config.h"
#include "Logger.h"

class PumpManager {
public:
  bool begin();
  void arm(bool armed);       // global auto-activation enable
  bool isArmed() const { return _armed; }
  void trigger();             // manual / automatic trigger
  void stop();
  //bool isRunning() const { return _running; }
  void update(AlarmState alarmState);  // call in loop — auto-triggers on DANGER if armed

private:
  bool _armed = true;
  bool _running = false;
  uint32_t _runStart = 0;
  static constexpr uint32_t MAX_RUN_MS = 30000;  // safety cutoff
};

extern PumpManager Pump;

#endif // PUMP_MANAGER_H