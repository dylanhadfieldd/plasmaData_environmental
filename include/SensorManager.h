#pragma once

#include <Arduino.h>
#include <Adafruit_SHTC3.h>
#include <Adafruit_VL6180X.h>

#include "AppTypes.h"

class SensorManager {
 public:
  bool begin();
  void update(uint32_t nowMs);
  const SensorSnapshot& snapshot() const { return snapshot_; }

 private:
  Adafruit_SHTC3 shtc3_;
  Adafruit_VL6180X vl6180x_;

  SensorSnapshot snapshot_;
  bool tempHumidityOnline_ = false;
  bool distanceOnline_ = false;
  bool tempHumidityPrimed_ = false;
  bool distancePrimed_ = false;
  uint32_t lastPollMs_ = 0;
};

