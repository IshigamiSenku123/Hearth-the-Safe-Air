#include "Logger.h"

Logger SysLog;

bool Logger::begin() {
  _mutex = xSemaphoreCreateMutex();
  return _mutex != nullptr;
}

void Logger::log(LogLevel level, LogCategory cat, const char* title, const char* fmt, ...) {
  if (!_mutex) return;
  xSemaphoreTake(_mutex, portMAX_DELAY);

  LogEvent& e = _ring[_head];
  e.timestamp = millis();
  e.level = level;
  e.cat = cat;
  e.ack = false;
  strlcpy(e.title, title, sizeof(e.title));

  va_list args;
  va_start(args, fmt);
  vsnprintf(e.message, sizeof(e.message), fmt, args);
  va_end(args);

  _head = (_head + 1) % LOG_RING_SIZE;
  if (_count < LOG_RING_SIZE) _count++;

  xSemaphoreGive(_mutex);

  // Also echo critical events to Serial immediately
  if (level == LogLevel::CRITICAL || level == LogLevel::ERROR) {
    Serial.printf("[LOG %s] %s: %s\n",
      level == LogLevel::CRITICAL ? "CRIT" : "ERR",
      title, e.message);
  }
}

void Logger::acknowledge(size_t idx) {
  if (!_mutex || idx >= _count) return;
  xSemaphoreTake(_mutex, portMAX_DELAY);
  size_t pos = (_head + LOG_RING_SIZE - 1 - idx) % LOG_RING_SIZE;
  _ring[pos].ack = true;
  xSemaphoreGive(_mutex);
}

const LogEvent* Logger::get(size_t idx) const {
  if (idx >= _count) return nullptr;
  size_t pos = (_head + LOG_RING_SIZE - 1 - idx) % LOG_RING_SIZE;
  return &_ring[pos];
}

void Logger::flush() {
  // Placeholder: bulk-upload to cloud
}

void Logger::toJson(char* buf, size_t len, size_t maxEvents) const {
  if (!_mutex) { buf[0] = '\0'; return; }
  xSemaphoreTake(_mutex, portMAX_DELAY);
  size_t written = 0;
  written += snprintf(buf + written, len - written, "[");
  size_t n = min(_count, maxEvents);
  for (size_t i = 0; i < n; i++) {
    const LogEvent* e = get(i);
    if (!e) break;
    if (i) written += snprintf(buf + written, len - written, ",");
    written += snprintf(buf + written, len - written,
      "{\"t\":%u,\"lvl\":%d,\"cat\":%d,\"title\":\"%s\",\"msg\":\"%s\",\"ack\":%s}",
      e->timestamp, (int)e->level, (int)e->cat, e->title, e->message,
      e->ack ? "true" : "false");
    if (written >= len - 1) break;
  }
  written += snprintf(buf + written, len - written, "]");
  xSemaphoreGive(_mutex);
}