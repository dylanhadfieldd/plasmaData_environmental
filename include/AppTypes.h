#pragma once

#include <Arduino.h>

struct SensorSnapshot {
  float temperatureC = NAN;
  float humidityPct = NAN;
  float distanceMm = NAN;
  bool tempHumidityOk = false;
  bool distanceOk = false;
  uint32_t sampleMs = 0;
};

struct TrendPoint {
  float temperatureC = NAN;
  float humidityPct = NAN;
  float distanceMm = NAN;
};

