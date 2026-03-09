#include "SensorManager.h"

#include <Wire.h>
#include <math.h>

#include "Config.h"

namespace {

bool validNumber(float v) {
  return !isnan(v) && !isinf(v);
}

}  // namespace

bool SensorManager::begin() {
  Wire.begin(cfg::kPinI2cSda, cfg::kPinI2cScl);

  pinMode(cfg::kPinDistanceXShut, OUTPUT);
  digitalWrite(cfg::kPinDistanceXShut, HIGH);
  delay(5);

  tempHumidityOnline_ = shtc3_.begin();
  distanceOnline_ = vl6180x_.begin();

  Serial.println(F("[SENSORS] ---- Startup ----"));
  Serial.printf("[SENSORS] SHTC3: %s\n", tempHumidityOnline_ ? "online" : "offline");
  Serial.printf("[SENSORS] VL6180X: %s\n", distanceOnline_ ? "online" : "offline");

  snapshot_.sampleMs = millis();
  return tempHumidityOnline_ || distanceOnline_;
}

void SensorManager::update(uint32_t nowMs) {
  if (nowMs - lastPollMs_ < cfg::kSensorPollMs) {
    return;
  }
  lastPollMs_ = nowMs;

  // Temp + humidity
  if (tempHumidityOnline_) {
    sensors_event_t humidity;
    sensors_event_t temp;

    if (shtc3_.getEvent(&humidity, &temp)) {
      if (validNumber(temp.temperature) && validNumber(humidity.relative_humidity)) {
        if (!tempHumidityPrimed_) {
          snapshot_.temperatureC = temp.temperature;
          snapshot_.humidityPct = humidity.relative_humidity;
          tempHumidityPrimed_ = true;
        } else {
          snapshot_.temperatureC =
              (cfg::kTempEmaAlpha * temp.temperature) +
              ((1.0f - cfg::kTempEmaAlpha) * snapshot_.temperatureC);
          snapshot_.humidityPct =
              (cfg::kHumidityEmaAlpha * humidity.relative_humidity) +
              ((1.0f - cfg::kHumidityEmaAlpha) * snapshot_.humidityPct);
        }
        snapshot_.tempHumidityOk = true;
      } else {
        snapshot_.tempHumidityOk = false;
      }
    } else {
      snapshot_.tempHumidityOk = false;
    }
  } else {
    snapshot_.tempHumidityOk = false;
  }

  // Distance
  if (distanceOnline_) {
    uint8_t rangeMm = vl6180x_.readRange();
    uint8_t status = vl6180x_.readRangeStatus();

    if (status == VL6180X_ERROR_NONE && rangeMm != 255U) {
      if (!distancePrimed_) {
        snapshot_.distanceMm = static_cast<float>(rangeMm);
        distancePrimed_ = true;
      } else {
        snapshot_.distanceMm =
            (cfg::kDistanceEmaAlpha * static_cast<float>(rangeMm)) +
            ((1.0f - cfg::kDistanceEmaAlpha) * snapshot_.distanceMm);
      }
      snapshot_.distanceOk = true;
    } else {
      snapshot_.distanceOk = false;
    }
  } else {
    snapshot_.distanceOk = false;
  }

  snapshot_.sampleMs = nowMs;
}

