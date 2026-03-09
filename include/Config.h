#pragma once

#include <Arduino.h>

namespace cfg {

// I2C (SHTC3 + VL6180X)
constexpr int kPinI2cSda = 21;
constexpr int kPinI2cScl = 22;
constexpr int kPinDistanceXShut = 27;

// TFT
constexpr int kPinTftBacklight = 15;
constexpr uint8_t kDisplayRotation = 1;

// Scheduler
constexpr uint32_t kSensorPollMs = 250;
constexpr uint32_t kTrendSampleMs = 1000;
constexpr uint32_t kUiFrameMs = 250;
constexpr uint32_t kSerialLogMs = 2000;

// Filtering
constexpr float kTempEmaAlpha = 0.20f;
constexpr float kHumidityEmaAlpha = 0.20f;
constexpr float kDistanceEmaAlpha = 0.30f;

// UI chart scales
constexpr float kTempMinC = 10.0f;
constexpr float kTempMaxC = 40.0f;
constexpr float kHumidityMinPct = 20.0f;
constexpr float kHumidityMaxPct = 90.0f;
constexpr float kDistanceMinMm = 0.0f;
constexpr float kDistanceMaxMm = 200.0f;

}  // namespace cfg

