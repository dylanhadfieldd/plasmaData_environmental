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
constexpr uint32_t kUiFrameMs = 500;
constexpr uint32_t kSerialLogMs = 2000;
constexpr uint32_t kTouchPollMs = 25;
constexpr uint32_t kTouchDebounceMs = 220;
constexpr uint8_t kTouchI2cAddress = 0x38;  // Common FT62xx/FT6336 address.
constexpr uint32_t kSparklineRefreshMs = 4000;

// Capacitive touch controller (CTP) helper pins.
// Wire CTP_SDA/CTP_SCL to the same I2C bus as sensors (GPIO21/GPIO22).
// Set to -1 to disable optional helper pins and use I2C-only touch probing.
constexpr int kPinTouchInt = -1;
constexpr int kPinTouchRst = -1;
constexpr uint32_t kTouchResetLowMs = 10;
constexpr uint32_t kTouchResetReadyMs = 60;

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
