#include "UiRenderer.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "Config.h"

namespace {

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

bool validNumber(float v) {
  return !isnan(v) && !isinf(v);
}

}  // namespace

bool UiRenderer::begin() {
  tft_.init();
  tft_.setRotation(cfg::kDisplayRotation);

  pinMode(cfg::kPinTftBacklight, OUTPUT);
  digitalWrite(cfg::kPinTftBacklight, HIGH);

  tft_.setTextSize(1);
  tft_.setTextFont(2);
  tft_.setTextDatum(TL_DATUM);
  tft_.fillScreen(TFT_BLACK);

  scaffoldDrawn_ = false;
  return true;
}

void UiRenderer::render(uint32_t nowMs, const SensorSnapshot& snapshot, const TrendBuffer& trends) {
  if (!scaffoldDrawn_) {
    drawScaffold_();
  }

  drawHeader_(nowMs);
  drawMetricCards_(snapshot, trends);
  drawFooter_(nowMs, snapshot);
}

void UiRenderer::drawScaffold_() {
  drawVerticalGradient_(0, 0, tft_.width(), tft_.height());
  scaffoldDrawn_ = true;
}

void UiRenderer::drawVerticalGradient_(int16_t x, int16_t y, int16_t w, int16_t h) {
  const int16_t safeH = (h <= 0) ? 1 : h;
  for (int16_t i = 0; i < safeH; ++i) {
    float t = (safeH == 1) ? 0.0f : static_cast<float>(i) / static_cast<float>(safeH - 1);
    uint8_t r = static_cast<uint8_t>(8.0f + ((20.0f - 8.0f) * t));
    uint8_t g = static_cast<uint8_t>(18.0f + ((10.0f - 18.0f) * t));
    uint8_t b = static_cast<uint8_t>(34.0f + ((12.0f - 34.0f) * t));
    tft_.drawFastHLine(x, y + i, w, rgb565(r, g, b));
  }
}

void UiRenderer::drawHeader_(uint32_t nowMs) {
  const int16_t margin = (tft_.width() < 360) ? 8 : 12;
  const int16_t headerH = (tft_.height() < 280) ? 44 : 52;

  tft_.fillRoundRect(margin, margin, tft_.width() - (2 * margin), headerH, 10, rgb565(12, 30, 56));
  tft_.drawRoundRect(margin, margin, tft_.width() - (2 * margin), headerH, 10, rgb565(56, 118, 182));

  tft_.setTextColor(rgb565(220, 243, 255), rgb565(12, 30, 56));
  tft_.setTextFont(2);
  tft_.setTextDatum(TL_DATUM);
  tft_.drawString("ESP32 ENVIRONMENT DASHBOARD", margin + 10, margin + 8);

  tft_.setTextColor(rgb565(168, 214, 238), rgb565(12, 30, 56));
  tft_.drawString("Sensor-only build: temp + humidity + distance", margin + 10, margin + 26);

  char uptime[20];
  formatUptime_(nowMs, uptime, sizeof(uptime));
  tft_.setTextDatum(TR_DATUM);
  tft_.setTextColor(rgb565(255, 252, 237), rgb565(12, 30, 56));
  tft_.drawString(uptime, tft_.width() - margin - 10, margin + 8);
  tft_.setTextDatum(TL_DATUM);
}

void UiRenderer::drawFooter_(uint32_t nowMs, const SensorSnapshot& snapshot) {
  const int16_t margin = (tft_.width() < 360) ? 8 : 12;
  const int16_t footerH = (tft_.height() < 280) ? 24 : 28;
  const int16_t y = tft_.height() - margin - footerH;

  tft_.fillRoundRect(margin, y, tft_.width() - (2 * margin), footerH, 8, rgb565(10, 23, 44));
  tft_.drawRoundRect(margin, y, tft_.width() - (2 * margin), footerH, 8, rgb565(46, 88, 140));

  const uint32_t ageMs = nowMs - snapshot.sampleMs;
  char line[80];
  if (snapshot.tempHumidityOk && snapshot.distanceOk) {
    snprintf(line, sizeof(line), "Sensors healthy | sample age: %lus", ageMs / 1000UL);
  } else if (!snapshot.tempHumidityOk && !snapshot.distanceOk) {
    snprintf(line, sizeof(line), "All sensors offline | check wiring and power");
  } else if (!snapshot.tempHumidityOk) {
    snprintf(line, sizeof(line), "SHTC3 offline | sample age: %lus", ageMs / 1000UL);
  } else {
    snprintf(line, sizeof(line), "VL6180X offline | sample age: %lus", ageMs / 1000UL);
  }

  tft_.setTextColor(rgb565(196, 225, 248), rgb565(10, 23, 44));
  tft_.setTextFont(2);
  tft_.setTextDatum(TL_DATUM);
  tft_.drawString(line, margin + 8, y + 6);
}

void UiRenderer::drawMetricCards_(const SensorSnapshot& snapshot, const TrendBuffer& trends) {
  const int16_t margin = (tft_.width() < 360) ? 8 : 12;
  const int16_t gap = margin;
  const int16_t headerH = (tft_.height() < 280) ? 44 : 52;
  const int16_t footerH = (tft_.height() < 280) ? 24 : 28;
  const int16_t top = margin + headerH + gap;
  const int16_t usableH = tft_.height() - top - footerH - (2 * margin) - (2 * gap);
  const int16_t cardH = usableH / 3;
  const int16_t cardW = tft_.width() - (2 * margin);

  int16_t y = top;
  drawMetricCard_(margin, y, cardW, cardH, MetricKind::Temperature, snapshot.temperatureC,
                  snapshot.tempHumidityOk, trends);
  y += cardH + gap;
  drawMetricCard_(margin, y, cardW, cardH, MetricKind::Humidity, snapshot.humidityPct,
                  snapshot.tempHumidityOk, trends);
  y += cardH + gap;
  drawMetricCard_(margin, y, cardW, cardH, MetricKind::Distance, snapshot.distanceMm,
                  snapshot.distanceOk, trends);
}

void UiRenderer::drawMetricCard_(int16_t x, int16_t y, int16_t w, int16_t h, MetricKind metric,
                                 float value, bool valid, const TrendBuffer& trends) {
  const char* title = "";
  const char* unit = "";
  float minValue = 0.0f;
  float maxValue = 1.0f;
  uint16_t accent = TFT_WHITE;
  metricMeta_(metric, title, unit, minValue, maxValue, accent);

  tft_.fillRoundRect(x, y, w, h, 10, rgb565(14, 27, 50));
  tft_.drawRoundRect(x, y, w, h, 10, rgb565(36, 74, 122));
  tft_.drawFastHLine(x + 2, y + 28, w - 4, rgb565(34, 64, 103));

  tft_.setTextDatum(TL_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(rgb565(193, 221, 241), rgb565(14, 27, 50));
  tft_.drawString(title, x + 10, y + 8);

  if (valid) {
    drawStatusBadge_(x + w - 68, y + 6, "LIVE", rgb565(27, 130, 84), rgb565(233, 252, 239));
  } else {
    drawStatusBadge_(x + w - 86, y + 6, "OFFLINE", rgb565(136, 38, 43), rgb565(255, 229, 229));
  }

  char valueText[24];
  formatMetricValue_(metric, value, valid, valueText, sizeof(valueText));

  if (h >= 80) {
    tft_.setTextFont(4);
  } else {
    tft_.setTextFont(2);
  }
  tft_.setTextColor(rgb565(247, 250, 253), rgb565(14, 27, 50));
  tft_.drawString(valueText, x + 10, y + 40);

  char unitText[12];
  snprintf(unitText, sizeof(unitText), "%s", unit);
  tft_.setTextFont(2);
  tft_.setTextColor(rgb565(151, 193, 223), rgb565(14, 27, 50));
  tft_.drawString(unitText, x + 10, y + h - 26);

  const int16_t barX = x + 10;
  const int16_t barY = y + h - 12;
  const int16_t barW = (w / 2) - 24;
  drawBar_(barX, barY, barW, 6, normalize_(value, minValue, maxValue), accent);

  const int16_t chartX = x + (w / 2);
  const int16_t chartY = y + 34;
  const int16_t chartW = (w / 2) - 12;
  const int16_t chartH = h - 42;
  drawSparkline_(chartX, chartY, chartW, chartH, metric, minValue, maxValue, accent, trends);
}

void UiRenderer::drawStatusBadge_(int16_t x, int16_t y, const char* label, uint16_t fillColor,
                                  uint16_t textColor) {
  const int16_t w = strlen(label) * 7 + 12;
  tft_.fillRoundRect(x, y, w, 16, 7, fillColor);
  tft_.setTextFont(1);
  tft_.setTextColor(textColor, fillColor);
  tft_.setTextDatum(TL_DATUM);
  tft_.drawString(label, x + 6, y + 4);
}

void UiRenderer::drawBar_(int16_t x, int16_t y, int16_t w, int16_t h, float normalized,
                          uint16_t fillColor) {
  if (w <= 0 || h <= 0) {
    return;
  }

  float clamped = normalized;
  if (clamped < 0.0f) clamped = 0.0f;
  if (clamped > 1.0f) clamped = 1.0f;

  tft_.fillRoundRect(x, y, w, h, 3, rgb565(20, 42, 71));
  int16_t fillW = static_cast<int16_t>(clamped * static_cast<float>(w));
  if (fillW > 0) {
    tft_.fillRoundRect(x, y, fillW, h, 3, fillColor);
  }
}

void UiRenderer::drawSparkline_(int16_t x, int16_t y, int16_t w, int16_t h, MetricKind metric,
                                float minValue, float maxValue, uint16_t color,
                                const TrendBuffer& trends) {
  if (w <= 4 || h <= 4) {
    return;
  }

  tft_.fillRect(x, y, w, h, rgb565(9, 21, 39));
  tft_.drawRect(x, y, w, h, rgb565(30, 56, 90));

  const size_t n = trends.count();
  if (n < 2) {
    tft_.setTextFont(1);
    tft_.setTextColor(rgb565(125, 164, 194), rgb565(9, 21, 39));
    tft_.drawString("Collecting...", x + 4, y + (h / 2) - 3);
    return;
  }

  int16_t prevX = -1;
  int16_t prevY = -1;
  bool prevValid = false;

  for (size_t i = 0; i < n; ++i) {
    TrendPoint point = trends.atOldest(i);
    float value = pickMetricValue_(point, metric);
    if (!validNumber(value)) {
      prevValid = false;
      continue;
    }

    float normalized = normalize_(value, minValue, maxValue);
    int16_t px = x + static_cast<int16_t>((static_cast<float>(i) / static_cast<float>(n - 1)) *
                                          static_cast<float>(w - 1));
    int16_t py = y + h - 1 - static_cast<int16_t>(normalized * static_cast<float>(h - 1));

    if (prevValid) {
      tft_.drawLine(prevX, prevY, px, py, color);
    }
    prevX = px;
    prevY = py;
    prevValid = true;
  }
}

float UiRenderer::pickMetricValue_(const TrendPoint& point, MetricKind metric) const {
  switch (metric) {
    case MetricKind::Temperature:
      return point.temperatureC;
    case MetricKind::Humidity:
      return point.humidityPct;
    case MetricKind::Distance:
      return point.distanceMm;
    default:
      return NAN;
  }
}

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

  snprintf(out, outSize, "UP %02lu:%02lu:%02lu", static_cast<unsigned long>(hours),
           static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
}
