// SafeAir Button Manager — 3 tactile buttons with debounce, long & double press
#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include "Config.h"

struct ButtonEventData {
  ButtonId id;
  ButtonEvent evt;
};

class ButtonManager {
public:
  bool begin();
  void update();  // call every INTERVAL_BUTTON_MS
  bool hasEvent() const;
  ButtonEventData getEvent();
  void clearEvents();

private:
  struct Btn {
    uint8_t pin;
    bool lastState;
    uint32_t pressTime;
    uint32_t releaseTime;
    bool waitingDouble;
  };
  Btn _btns[3];
  ButtonEventData _evtBuf[8];
  size_t _evtHead = 0, _evtTail = 0, _evtCount = 0;

  void _poll(Btn& b, ButtonId id);
  void _push(ButtonId id, ButtonEvent e);
};

extern ButtonManager Buttons;

#endif