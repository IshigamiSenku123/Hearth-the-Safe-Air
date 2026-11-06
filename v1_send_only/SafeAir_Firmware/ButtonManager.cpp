#include "ButtonManager.h"

ButtonManager Buttons;

bool ButtonManager::begin() {
  pinMode(PIN_BTN_PREV,   INPUT_PULLUP);
  pinMode(PIN_BTN_SELECT, INPUT_PULLUP);
  pinMode(PIN_BTN_NEXT,   INPUT_PULLUP);

  _btns[0] = {PIN_BTN_PREV,   true, 0, 0, false};
  _btns[1] = {PIN_BTN_SELECT, true, 0, 0, false};
  _btns[2] = {PIN_BTN_NEXT,   true, 0, 0, false};
  return true;
}

void ButtonManager::update() {
  _poll(_btns[0], ButtonId::PREV);
  _poll(_btns[1], ButtonId::SELECT);
  _poll(_btns[2], ButtonId::NEXT);
}

void ButtonManager::_poll(Btn& b, ButtonId id) {
  bool raw = digitalRead(b.pin);
  uint32_t now = millis();

  if (raw != b.lastState) {
    if (!raw) { // pressed (active LOW)
      b.pressTime = now;
      b.waitingDouble = false;
    } else {    // released
      uint32_t dur = now - b.pressTime;
      if (dur >= DEBOUNCE_MS) {
        if (dur >= LONG_PRESS_MS) {
          _push(id, ButtonEvent::LONG_PRESS);
        } else if (b.waitingDouble) {
          _push(id, ButtonEvent::DOUBLE_PRESS);
          b.waitingDouble = false;
        } else {
          b.waitingDouble = true;
          b.releaseTime = now;
        }
      }
    }
    b.lastState = raw;
  }

  // Expire double-press window
  if (b.waitingDouble && (now - b.releaseTime > DOUBLE_PRESS_MS)) {
    b.waitingDouble = false;
    _push(id, ButtonEvent::PRESS);
  }
}

void ButtonManager::_push(ButtonId id, ButtonEvent e) {
  if (_evtCount >= 8) return;  // overflow drop
  _evtBuf[_evtHead] = {id, e};
  _evtHead = (_evtHead + 1) % 8;
  _evtCount++;
}

bool ButtonManager::hasEvent() const {
  return _evtCount > 0;
}

ButtonEventData ButtonManager::getEvent() {
  ButtonEventData ev = {ButtonId::NONE, ButtonEvent::NONE};
  if (_evtCount == 0) return ev;
  ev = _evtBuf[_evtTail];
  _evtTail = (_evtTail + 1) % 8;
  _evtCount--;
  return ev;
}

void ButtonManager::clearEvents() {
  _evtHead = _evtTail = _evtCount = 0;
}