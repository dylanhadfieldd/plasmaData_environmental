#include "UiRenderer.h"

#include <stdio.h>

#include "UiRendererSupport.h"

using namespace ui_render;

void UiRenderer::drawScaffold_() {
  drawVerticalGradient_(0, 0, tft_.width(), tft_.height());
  drawHeaderFrame_();
  drawPageFrame_();
  drawFooterFrame_();

  scaffoldDrawn_ = true;
  resetDynamicCaches_();
}

void UiRenderer::drawPageFrame_() {
  switch (currentPage_) {
    case PageKind::Logo:
      drawLogoFrame_();
      break;
    case PageKind::Dashboard:
      drawMetricCardsFrame_();
      break;
    case PageKind::Trends:
      drawTrendsFrame_();
      break;
    case PageKind::Logging:
      drawLoggingFrame_();
      break;
    default:
      drawMetricCardsFrame_();
      break;
  }
}

void UiRenderer::drawVerticalGradient_(int16_t x, int16_t y, int16_t w, int16_t h) {
  const int16_t safeH = (h <= 0) ? 1 : h;
  for (int16_t i = 0; i < safeH; ++i) {
    const float t = (safeH == 1) ? 0.0f : static_cast<float>(i) / static_cast<float>(safeH - 1);
    const uint8_t r = static_cast<uint8_t>(8.0f + ((20.0f - 8.0f) * t));
    const uint8_t g = static_cast<uint8_t>(18.0f + ((10.0f - 18.0f) * t));
    const uint8_t b = static_cast<uint8_t>(34.0f + ((12.0f - 34.0f) * t));
    tft_.drawFastHLine(x, y + i, w, rgb565(r, g, b));
  }
}

void UiRenderer::drawHeaderFrame_() {
  const int16_t margin = uiMargin(tft_.width());
  const int16_t headerH = uiHeaderH(tft_.height());
  const uint16_t headerBg = colorHeaderBg();

  tft_.fillRoundRect(margin, margin, tft_.width() - (2 * margin), headerH, 10, headerBg);
  tft_.drawRoundRect(margin, margin, tft_.width() - (2 * margin), headerH, 10,
                     colorHeaderBorder());
}

void UiRenderer::drawFooterFrame_() {
  const int16_t margin = uiMargin(tft_.width());
  const int16_t footerH = uiFooterH(tft_.height());
  const int16_t y = tft_.height() - margin - footerH;

  tft_.fillRoundRect(margin, y, tft_.width() - (2 * margin), footerH, 8, colorFooterBg());
  tft_.drawRoundRect(margin, y, tft_.width() - (2 * margin), footerH, 8, rgb565(46, 88, 140));
}

void UiRenderer::drawLogoFrame_() {
  const int16_t margin = uiMargin(tft_.width());
  const int16_t gap = uiGap(tft_.width());
  const int16_t headerH = uiHeaderH(tft_.height());
  const int16_t footerH = uiFooterH(tft_.height());
  const int16_t bodyX = margin;
  const int16_t bodyY = margin + headerH + gap;
  const int16_t bodyW = tft_.width() - (2 * margin);
  const int16_t bodyH = tft_.height() - bodyY - footerH - margin - gap;
  const uint16_t cardBg = colorPanelBg();

  tft_.fillRoundRect(bodyX, bodyY, bodyW, bodyH, 10, cardBg);
  tft_.drawRoundRect(bodyX, bodyY, bodyW, bodyH, 10, colorPanelBorder());
}

void UiRenderer::drawMetricCardsFrame_() {
  drawMetricCardFrame_(metricCardGeometry_(MetricKind::Temperature), MetricKind::Temperature);
  drawMetricCardFrame_(metricCardGeometry_(MetricKind::Humidity), MetricKind::Humidity);
  drawMetricCardFrame_(metricCardGeometry_(MetricKind::Distance), MetricKind::Distance);
}

void UiRenderer::drawTrendsFrame_() {
  const int16_t margin = uiMargin(tft_.width());
  const int16_t gap = uiGap(tft_.width());
  const int16_t headerH = uiHeaderH(tft_.height());
  const int16_t footerH = uiFooterH(tft_.height());
  const int16_t bodyX = margin;
  const int16_t bodyY = margin + headerH + gap;
  const int16_t bodyW = tft_.width() - (2 * margin);
  const int16_t bodyH = tft_.height() - bodyY - footerH - margin - gap;
  const uint16_t cardBg = colorPanelBg();

  tft_.fillRoundRect(bodyX, bodyY, bodyW, bodyH, 10, cardBg);
  tft_.drawRoundRect(bodyX, bodyY, bodyW, bodyH, 10, colorPanelBorder());
}

void UiRenderer::drawLoggingFrame_() {
  const int16_t margin = uiMargin(tft_.width());
  const int16_t gap = uiGap(tft_.width());
  const int16_t headerH = uiHeaderH(tft_.height());
  const int16_t footerH = uiFooterH(tft_.height());
  const int16_t bodyX = margin;
  const int16_t bodyY = margin + headerH + gap;
  const int16_t bodyW = tft_.width() - (2 * margin);
  const int16_t bodyH = tft_.height() - bodyY - footerH - margin - gap;
  const uint16_t cardBg = colorPanelBg();

  tft_.fillRoundRect(bodyX, bodyY, bodyW, bodyH, 10, cardBg);
  tft_.drawRoundRect(bodyX, bodyY, bodyW, bodyH, 10, colorPanelBorder());
}

void UiRenderer::drawMetricCardFrame_(const CardGeometry& geometry, MetricKind metric) {
  const uint16_t cardBg = colorPanelBg();
  const char* title = "";
  const char* unit = "";
  float minValue = 0.0f;
  float maxValue = 1.0f;
  uint16_t accent = TFT_WHITE;
  metricMeta_(metric, title, unit, minValue, maxValue, accent);
  (void)minValue;
  (void)maxValue;

  tft_.fillRoundRect(geometry.x, geometry.y, geometry.w, geometry.h, 10, cardBg);
  tft_.drawRoundRect(geometry.x, geometry.y, geometry.w, geometry.h, 10, colorPanelBorder());
  tft_.drawFastHLine(geometry.x + 2, geometry.y + 24, geometry.w - 4, colorGrid());

  tft_.setTextDatum(TL_DATUM);
  tft_.setTextSize(1);
  tft_.setFreeFont(&FreeSansBold9pt7b);
  tft_.setTextColor(rgb565(193, 221, 241), cardBg);
  char titleLine[48];
  if (unit != nullptr && unit[0] != '\0') {
    snprintf(titleLine, sizeof(titleLine), "%s (%s)", title, unit);
  } else {
    snprintf(titleLine, sizeof(titleLine), "%s", title);
  }
  tft_.drawString(titleLine, geometry.x + 10, geometry.y + 6);
  applyUiFont(tft_, UiFontRole::Small);
}

UiRenderer::CardGeometry UiRenderer::metricCardGeometry_(MetricKind metric) {
  const int16_t margin = uiMargin(tft_.width());
  const int16_t gap = uiGap(tft_.width());
  const int16_t headerH = uiHeaderH(tft_.height());
  const int16_t footerH = uiFooterH(tft_.height());
  const int16_t top = margin + headerH + gap;
  const int16_t usableH = tft_.height() - top - footerH - (2 * margin) - (2 * gap);
  const int16_t cardH = usableH / 3;
  const int16_t cardW = tft_.width() - (2 * margin);
  const int16_t index = metricIndex_(metric);

  CardGeometry geometry;
  geometry.x = margin;
  geometry.y = top + (index * (cardH + gap));
  geometry.w = cardW;
  geometry.h = cardH;
  return geometry;
}

int UiRenderer::metricIndex_(MetricKind metric) const {
  switch (metric) {
    case MetricKind::Temperature:
      return 0;
    case MetricKind::Humidity:
      return 1;
    case MetricKind::Distance:
      return 2;
    default:
      return 0;
  }
}
