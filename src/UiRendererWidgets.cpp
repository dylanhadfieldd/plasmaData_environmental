#include "UiRenderer.h"

#include <stdio.h>
#include <string.h>

#include "UiRendererSupport.h"

using namespace ui_render;

void UiRenderer::drawMetricCardDynamic_(const CardGeometry& geometry, MetricKind metric,
                                        float value, bool valid) {
  const char* title = "";
  const char* unit = "";
  float minValue = 0.0f;
  float maxValue = 1.0f;
  uint16_t accent = TFT_WHITE;
  metricMeta_(metric, title, unit, minValue, maxValue, accent);
  (void)title;

  char valueText[24];
  formatMetricValue_(metric, value, valid, valueText, sizeof(valueText));

  int16_t barX = geometry.x + ((geometry.w < 300) ? 146 : 170);
  int16_t barW = (geometry.x + geometry.w - 10) - barX;
  if (barW < 40) {
    barX = geometry.x + 10;
    barW = geometry.w - 20;
  }
  int16_t barFillW = 0;
  if (barW > 0 && valid && validNumber(value)) {
    float normalized = normalize_(value, minValue, maxValue);
    barFillW = static_cast<int16_t>(normalized * static_cast<float>(barW));
    if (barFillW < 0) barFillW = 0;
    if (barFillW > barW) barFillW = barW;
  }

  MetricRenderCache& cache = metricCache_[metricIndex_(metric)];
  const bool statusChanged = !cache.initialized || (cache.valid != valid);
  const bool valueChanged = !cache.initialized || (strcmp(cache.valueText, valueText) != 0);
  const bool barChanged = !cache.initialized || (cache.barFillW != barFillW);

  if (!statusChanged && !valueChanged && !barChanged) {
    return;
  }

  const uint16_t cardBg = colorPanelBg();

  if (valueChanged || !cache.initialized) {
    const int16_t valueX = geometry.x + 10;
    const int16_t valueY = geometry.y + 28;
    tft_.setTextSize(1);
    tft_.setFreeFont(&FreeSansBold9pt7b);
    const int16_t valueH = static_cast<int16_t>(tft_.fontHeight()) + 2;
    tft_.fillRect(valueX, valueY, barX - valueX - 4, valueH, cardBg);

    tft_.setTextColor(rgb565(255, 252, 237), cardBg);
    tft_.setTextDatum(TL_DATUM);
    tft_.drawString(valueText, valueX, valueY);
    applyUiFont(tft_, UiFontRole::Small);
  }

  if (barChanged || !cache.initialized) {
    const int16_t barY = geometry.y + geometry.h - 7;
    drawBar_(barX, barY, barW, 6, normalize_(value, minValue, maxValue), accent);
  }

  if (statusChanged || !cache.initialized) {
    const int16_t badgeClearX = geometry.x + geometry.w - 94;
    const int16_t badgeClearY = geometry.y + 4;
    const int16_t badgeClearH = uiFontHeight(tft_, UiFontRole::Small) + 8;
    tft_.fillRect(badgeClearX, badgeClearY, 90, badgeClearH, cardBg);

    if (valid) {
      drawStatusBadge_(geometry.x + geometry.w - 84, geometry.y + 6, "ONLINE", colorLiveBg(),
                       colorLiveText());
    } else {
      drawStatusBadge_(geometry.x + geometry.w - 88, geometry.y + 6, "OFFLINE",
                       colorOfflineBg(), colorOfflineText());
    }
  }

  cache.initialized = true;
  cache.valid = valid;
  cache.barFillW = barFillW;
  strncpy(cache.valueText, valueText, sizeof(cache.valueText) - 1);
  cache.valueText[sizeof(cache.valueText) - 1] = '\0';
}

void UiRenderer::drawStatusBadge_(int16_t x, int16_t y, const char* label, uint16_t fillColor,
                                  uint16_t textColor) {
  applyUiFont(tft_, UiFontRole::Small);
  const int16_t padX = 4;
  const int16_t padY = 1;
  const int16_t w = static_cast<int16_t>(tft_.textWidth(label) + (padX * 2));
  const int16_t h = static_cast<int16_t>(tft_.fontHeight() + (padY * 2));
  tft_.fillRoundRect(x, y, w, h, 7, fillColor);
  tft_.setTextColor(textColor, fillColor);
  tft_.setTextDatum(TL_DATUM);
  tft_.drawString(label, x + padX, y + padY);
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

void UiRenderer::drawCombinedTrendsChart_(int16_t x, int16_t y, int16_t w, int16_t h,
                                          const TrendBuffer& trends) {
  if (w <= 4 || h <= 4) {
    return;
  }

  const uint16_t panelBg = colorChartBg();
  const uint16_t plotBg = rgb565(11, 24, 43);
  const uint16_t frame = rgb565(34, 64, 103);
  const uint16_t grid = rgb565(20, 40, 66);
  const uint16_t tempColor = rgb565(255, 159, 67);
  const uint16_t humidityColor = rgb565(0, 200, 255);
  const uint16_t distanceColor = rgb565(126, 255, 163);

  tft_.fillRoundRect(x, y, w, h, 6, panelBg);
  tft_.drawRoundRect(x, y, w, h, 6, frame);

  const int16_t legendH = (h >= 54) ? 12 : 10;
  const int16_t plotX = x + 2;
  const int16_t plotY = y + legendH + 1;
  const int16_t plotW = w - 4;
  const int16_t plotH = h - legendH - 3;
  if (plotW <= 4 || plotH <= 4) {
    return;
  }

  const int16_t legendY = y + 2;
  tft_.drawFastHLine(x + 6, legendY + 3, 10, tempColor);
  tft_.drawFastHLine(x + 48, legendY + 3, 10, humidityColor);
  tft_.drawFastHLine(x + 88, legendY + 3, 10, distanceColor);
  applyUiFont(tft_, UiFontRole::Small);
  tft_.setTextDatum(TL_DATUM);
  tft_.setTextColor(rgb565(202, 226, 244), panelBg);
  tft_.drawString("Temp", x + 18, legendY);
  tft_.drawString("RH", x + 60, legendY);
  tft_.drawString("Dist", x + 100, legendY);

  tft_.fillRect(plotX, plotY, plotW, plotH, plotBg);
  tft_.drawRect(plotX, plotY, plotW, plotH, frame);

  for (int i = 1; i <= 3; ++i) {
    const int16_t gy = plotY + static_cast<int16_t>((static_cast<float>(i) / 4.0f) *
                                                     static_cast<float>(plotH - 1));
    tft_.drawFastHLine(plotX + 1, gy, plotW - 2, grid);
    const int16_t gx = plotX + static_cast<int16_t>((static_cast<float>(i) / 4.0f) *
                                                     static_cast<float>(plotW - 1));
    tft_.drawFastVLine(gx, plotY + 1, plotH - 2, grid);
  }

  const size_t n = trends.count();
  if (n < 2) {
    applyUiFont(tft_, UiFontRole::Small);
    tft_.setTextDatum(MC_DATUM);
    tft_.setTextColor(rgb565(125, 164, 194), plotBg);
    tft_.drawString("Collecting samples...", plotX + (plotW / 2), plotY + (plotH / 2));
    tft_.setTextDatum(TL_DATUM);
    return;
  }

  auto drawSeriesSegment = [&](int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    tft_.drawLine(x1, y1, x2, y2, color);
    if (plotH > 26) {
      tft_.drawLine(x1, y1 + 1, x2, y2 + 1, color);
    }
  };

  int16_t prevTempX = -1;
  int16_t prevTempY = -1;
  bool prevTempValid = false;
  int16_t lastTempX = -1;
  int16_t lastTempY = -1;

  int16_t prevHumidityX = -1;
  int16_t prevHumidityY = -1;
  bool prevHumidityValid = false;
  int16_t lastHumidityX = -1;
  int16_t lastHumidityY = -1;

  int16_t prevDistanceX = -1;
  int16_t prevDistanceY = -1;
  bool prevDistanceValid = false;
  int16_t lastDistanceX = -1;
  int16_t lastDistanceY = -1;

  for (size_t i = 0; i < n; ++i) {
    const TrendPoint point = trends.atOldest(i);
    const int16_t px = plotX + static_cast<int16_t>(
                                   (static_cast<float>(i) / static_cast<float>(n - 1)) *
                                   static_cast<float>(plotW - 1));

    if (validNumber(point.temperatureC)) {
      const float normTemp = normalize_(point.temperatureC, cfg::kTempMinC, cfg::kTempMaxC);
      const int16_t pyTemp = plotY + plotH - 1 -
                             static_cast<int16_t>(normTemp * static_cast<float>(plotH - 1));
      if (prevTempValid) {
        drawSeriesSegment(prevTempX, prevTempY, px, pyTemp, tempColor);
      }
      prevTempX = px;
      prevTempY = pyTemp;
      prevTempValid = true;
      lastTempX = px;
      lastTempY = pyTemp;
    } else {
      prevTempValid = false;
    }

    if (validNumber(point.humidityPct)) {
      const float normHumidity =
          normalize_(point.humidityPct, cfg::kHumidityMinPct, cfg::kHumidityMaxPct);
      const int16_t pyHumidity = plotY + plotH - 1 -
                                 static_cast<int16_t>(normHumidity * static_cast<float>(plotH - 1));
      if (prevHumidityValid) {
        drawSeriesSegment(prevHumidityX, prevHumidityY, px, pyHumidity, humidityColor);
      }
      prevHumidityX = px;
      prevHumidityY = pyHumidity;
      prevHumidityValid = true;
      lastHumidityX = px;
      lastHumidityY = pyHumidity;
    } else {
      prevHumidityValid = false;
    }

    if (validNumber(point.distanceMm)) {
      const float normDistance =
          normalize_(point.distanceMm, cfg::kDistanceMinMm, cfg::kDistanceMaxMm);
      const int16_t pyDistance = plotY + plotH - 1 -
                                 static_cast<int16_t>(normDistance * static_cast<float>(plotH - 1));
      if (prevDistanceValid) {
        drawSeriesSegment(prevDistanceX, prevDistanceY, px, pyDistance, distanceColor);
      }
      prevDistanceX = px;
      prevDistanceY = pyDistance;
      prevDistanceValid = true;
      lastDistanceX = px;
      lastDistanceY = pyDistance;
    } else {
      prevDistanceValid = false;
    }
  }

  if (lastTempX >= 0) {
    tft_.fillCircle(lastTempX, lastTempY, 2, tempColor);
  }
  if (lastHumidityX >= 0) {
    tft_.fillCircle(lastHumidityX, lastHumidityY, 2, humidityColor);
  }
  if (lastDistanceX >= 0) {
    tft_.fillCircle(lastDistanceX, lastDistanceY, 2, distanceColor);
  }

  tft_.setTextDatum(TL_DATUM);
  applyUiFont(tft_, UiFontRole::Small);
}
