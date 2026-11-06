// SafeAir Logger — ring-buffer event log
#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "Config.h"

enum class LogLevel { INFO, WARNING, ERROR, CRITICAL };
enum class LogCategory { SYSTEM, SENSOR, NETWORK, ALARM, CLOUD, USER };

struct LogEvent {
  uint32_t timestamp;
  LogLevel level;
  LogCategory cat;
  char title[32];
  char message[64];
  bool ack;        // acknowledged by user?
};

class Logger {
public:
  bool begin();
  void log(LogLevel level, LogCategory cat, const char* title, const char* fmt, ...);
  void acknowledge(size_t idx);
  size_t count() const { return _count; }
  const LogEvent* get(size_t idx) const;  // 0 = newest
  void flush();  // push to cloud if connected
  void toJson(char* buf, size_t len, size_t maxEvents = 20) const;

private:
  LogEvent _ring[LOG_RING_SIZE];
  size_t   _head = 0;
  size_t   _count = 0;
  SemaphoreHandle_t _mutex;
};

extern Logger SysLog;

#endif