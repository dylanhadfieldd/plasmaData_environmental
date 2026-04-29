#include "UiRenderer.h"

#include <stdio.h>

#include "UiRendererSupport.h"

using namespace ui_render;

void UiRenderer::formatMetricValue_(MetricKind metric, float value, bool valid, char* out,
                                    size_t outSize) const {
  if (!valid || !validNumber(value)) {
    snprintf(out, outSize, "--");
    return;
  }

  switch (metric) {
    case MetricKind::Temperature:
      snprintf(out, outSize, "%.1f", value);
      return;
    case MetricKind::Humidity:
      snprintf(out, outSize, "%.1f", value);
      return;
    case MetricKind::Distance:
      snprintf(out, outSize, "%.0f", value);
      return;
    default:
      snprintf(out, outSize, "--");
      return;
  }
}

float UiRenderer::normalize_(float value, float minValue, float maxValue) const {
  if (!validNumber(value) || maxValue <= minValue) {
    return 0.0f;
  }

  float normalized = (value - minValue) / (maxValue - minValue);
  if (normalized < 0.0f) normalized = 0.0f;
  if (normalized > 1.0f) normalized = 1.0f;
  return normalized;
}

void UiRenderer::metricMeta_(MetricKind metric, const char*& title, const char*& unit,
                             float& minValue, float& maxValue, uint16_t& accentColor) const {
  switch (metric) {
    case MetricKind::Temperature:
      title = "Temperature";
      unit = "deg C";
      minValue = cfg::kTempMinC;
      maxValue = cfg::kTempMaxC;
      accentColor = rgb565(255, 159, 67);
      return;
    case MetricKind::Humidity:
      title = "Humidity";
      unit = "% RH";
      minValue = cfg::kHumidityMinPct;
      maxValue = cfg::kHumidityMaxPct;
      accentColor = rgb565(0, 200, 255);
      return;
    case MetricKind::Distance:
      title = "Distance";
      unit = "mm";
      minValue = cfg::kDistanceMinMm;
      maxValue = cfg::kDistanceMaxMm;
      accentColor = rgb565(126, 255, 163);
      return;
    default:
      title = "Metric";
      unit = "";
      minValue = 0.0f;
      maxValue = 1.0f;
      accentColor = TFT_WHITE;
      return;
  }
}

void UiRenderer::formatUptime_(uint32_t nowMs, char* out, size_t outSize) const {
  uint32_t totalSeconds = nowMs / 1000UL;
  uint32_t hours = totalSeconds / 3600UL;
  uint32_t minutes = (totalSeconds % 3600UL) / 60UL;
  uint32_t seconds = totalSeconds % 60UL;

  snprintf(out, outSize, "Uptime %02lu:%02lu:%02lu", static_cast<unsigned long>(hours),
           static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
}
