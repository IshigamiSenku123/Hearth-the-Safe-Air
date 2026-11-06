// SafeAir Sensor Manager — MQ-2, MQ-4, DHT11, smoke placeholder
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>
#include "Config.h"
#include "StorageManager.h"

struct SensorReading {
  float value;      // raw or computed
  float avg40;      // rolling average
  float peak40;     // rolling peak
  bool  valid;
  uint32_t lastRead;
};

struct SensorSnapshot {
  SensorReading lpg;      // from MQ-2
  SensorReading methane;  // from MQ-4
  SensorReading temp;     // from DHT11
  SensorReading humid;    // from DHT11
  SensorReading smoke;    // from MQ-2 secondary or placeholder
};

class SensorManager {
public:
  bool begin();
  void calibrate();              // blocking calibration on first boot
  void update();                 // call every INTERVAL_SENSOR_MS
  const SensorSnapshot& read() const { return _snap; }
  bool isCalibrated() const { return _calibrated; }

  // Helpers for alarm logic
  float getLPG()      const { return _snap.lpg.value; }
  float getMethane()  const { return _snap.methane.value; }
  float getTemp()     const { return _snap.temp.value; }
  float getHumidity() const { return _snap.humid.value; }
  float getSmoke()    const { return _snap.smoke.value; }

private:
  DHT _dht{PIN_DHT, DHT11};
  SensorSnapshot _snap{};
  bool _calibrated = false;

  // Calibration resistances (clean air)
  float _mq2Ro = 10.0f;
  float _mq4Ro = 10.0f;

  // Rolling buffers for avg/peak (40 samples ≈ 40s)
  static constexpr size_t HIST_SIZE = 40;
  float _histLpg[HIST_SIZE] = {0};
  float _histMethane[HIST_SIZE] = {0};
  float _histTemp[HIST_SIZE] = {0};
  float _histSmoke[HIST_SIZE] = {0};
  size_t _histIdx = 0;
  size_t _histCount = 0;

  float _readMQ(int pin, float ro, float cleanAirRatio);
  float _ppmMQ2(float rs_ro);
  float _ppmMQ4(float rs_ro);
  void _pushHist(float* buf, float v);
  float _avg(const float* buf) const;
  float _peak(const float* buf) const;
};

extern SensorManager Sensors;

#endif