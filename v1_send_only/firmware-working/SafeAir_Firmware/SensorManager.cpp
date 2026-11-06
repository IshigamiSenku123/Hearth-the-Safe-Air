#include "SensorManager.h"
#include"logger.h"
SensorManager Sensors;

static constexpr float MQ2_LPG_M    = -0.47f;
static constexpr float MQ2_LPG_B    = 1.31f;
static constexpr float MQ4_CH4_M    = -0.36f;
static constexpr float MQ4_CH4_B    = 1.20f;

static inline float readADC(int pin) {
  int raw = analogRead(pin);
  return (float)raw / 4095.0f * 3.3f;
}

bool SensorManager::begin() {
  pinMode(PIN_MQ2_A, INPUT);
  pinMode(PIN_MQ4_A, INPUT);
  _dht.begin();

  bool done = false;
  Storage.loadCalibration(_mq2Ro, _mq4Ro, done);
  _calibrated = done;

  if (!_calibrated) {
    Serial.println("[SENSOR] Calibration required — starting clean-air sampling...");
    calibrate();
  } else {
    Serial.printf("[SENSOR] Loaded calibration MQ2_Ro=%.2f  MQ4_Ro=%.2f\n", _mq2Ro, _mq4Ro);
  }
  return true;
}

void SensorManager::calibrate() {
  float sum2 = 0, sum4 = 0;
  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    float v2 = readADC(PIN_MQ2_A);
    float v4 = readADC(PIN_MQ4_A);
    sum2 += v2;
    sum4 += v4;
    delay(CALIBRATION_INTERVAL);
  }
  _mq2Ro = sum2 / CALIBRATION_SAMPLES;
  _mq4Ro = sum4 / CALIBRATION_SAMPLES;

  if (_mq2Ro < 0.01f) _mq2Ro = 0.01f;
  if (_mq4Ro < 0.01f) _mq4Ro = 0.01f;

  Storage.saveCalibration(_mq2Ro, _mq4Ro);
  _calibrated = true;

  SysLog.log(LogLevel::INFO, LogCategory::SENSOR,
    "Calibration complete",
    "MQ2_Ro=%.3f  MQ4_Ro=%.3f", _mq2Ro, _mq4Ro);
}

void SensorManager::update() {
  float v2 = readADC(PIN_MQ2_A);
  float ppm2 = _ppmMQ2(v2 / _mq2Ro);
  _snap.lpg.value = ppm2;
  _snap.lpg.valid = true;
  _pushHist(_histLpg, ppm2);
  _snap.lpg.avg40 = _avg(_histLpg);
  _snap.lpg.peak40 = _peak(_histLpg);

  float v4 = readADC(PIN_MQ4_A);
  float ppm4 = _ppmMQ4(v4 / _mq4Ro);
  _snap.methane.value = ppm4;
  _snap.methane.valid = true;
  _pushHist(_histMethane, ppm4);
  _snap.methane.avg40 = _avg(_histMethane);
  _snap.methane.peak40 = _peak(_histMethane);

  float t = _dht.readTemperature();
  float h = _dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    _snap.temp.value = t;
    _snap.temp.valid = true;
    _pushHist(_histTemp, t);
    _snap.temp.avg40 = _avg(_histTemp);
    _snap.temp.peak40 = _peak(_histTemp);

    _snap.humid.value = h;
    _snap.humid.valid = true;
  }

  float smokePpm = ppm2 * 0.6f;
  _snap.smoke.value = smokePpm;
  _snap.smoke.valid = true;
  _pushHist(_histSmoke, smokePpm);
  _snap.smoke.avg40 = _avg(_histSmoke);
  _snap.smoke.peak40 = _peak(_histSmoke);

  _snap.lpg.lastRead = _snap.methane.lastRead = _snap.temp.lastRead = millis();
}

float SensorManager::_ppmMQ2(float rs_ro) {
  if (rs_ro <= 0) rs_ro = 0.001f;
  float logPpm = (log10f(rs_ro) - MQ2_LPG_B) / MQ2_LPG_M;
  float ppm = powf(10.0f, logPpm);
  return constrain(ppm, 0.0f, (float)MQ2_MAX_PPM);
}

float SensorManager::_ppmMQ4(float rs_ro) {
  if (rs_ro <= 0) rs_ro = 0.001f;
  float logPpm = (log10f(rs_ro) - MQ4_CH4_B) / MQ4_CH4_M;
  float ppm = powf(10.0f, logPpm);
  return constrain(ppm, 0.0f, (float)MQ4_MAX_PPM);
}

void SensorManager::_pushHist(float* buf, float v) {
  buf[_histIdx] = v;
  _histIdx = (_histIdx + 1) % HIST_SIZE;
  if (_histCount < HIST_SIZE) _histCount++;
}

float SensorManager::_avg(const float* buf) const {
  if (_histCount == 0) return 0;
  float s = 0;
  for (size_t i = 0; i < _histCount; i++) s += buf[i];
  return s / _histCount;
}

float SensorManager::_peak(const float* buf) const {
  if (_histCount == 0) return 0;
  float m = buf[0];
  for (size_t i = 1; i < _histCount; i++) if (buf[i] > m) m = buf[i];
  return m;
}