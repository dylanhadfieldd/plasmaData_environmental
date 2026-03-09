#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "AppTypes.h"
#include "TrendBuffer.h"

class UiRenderer {
 public:
  bool begin();
  void render(uint32_t nowMs, const SensorSnapshot& snapshot, const TrendBuffer& trends);

 private:
  enum class MetricKind : uint8_t {
    Temperature,
    Humidity,
    Distance,
  };

  TFT_eSPI tft_;
  bool scaffoldDrawn_ = false;

  void drawScaffold_();
  void drawVerticalGradient_(int16_t x, int16_t y, int16_t w, int16_t h);
  void drawHeader_(uint32_t nowMs);
  void drawFooter_(uint32_t nowMs, const SensorSnapshot& snapshot);
  void drawMetricCards_(const SensorSnapshot& snapshot, const TrendBuffer& trends);
  void drawMetricCard_(int16_t x, int16_t y, int16_t w, int16_t h, MetricKind metric,
                       float value, bool valid, const TrendBuffer& trends);
  void drawStatusBadge_(int16_t x, int16_t y, const char* label, uint16_t fillColor,
                        uint16_t textColor);
  void drawBar_(int16_t x, int16_t y, int16_t w, int16_t h, float normalized,
                uint16_t fillColor);
  void drawSparkline_(int16_t x, int16_t y, int16_t w, int16_t h, MetricKind metric,
                      float minValue, float maxValue, uint16_t color, const TrendBuffer& trends);

  float pickMetricValue_(const TrendPoint& point, MetricKind metric) const;
  void formatMetricValue_(MetricKind metric, float value, bool valid, char* out,
                          size_t outSize) const;
  float normalize_(float value, float minValue, float maxValue) const;
  void metricMeta_(MetricKind metric, const char*& title, const char*& unit, float& minValue,
                   float& maxValue, uint16_t& accentColor) const;
  void formatUptime_(uint32_t nowMs, char* out, size_t outSize) const;
};

