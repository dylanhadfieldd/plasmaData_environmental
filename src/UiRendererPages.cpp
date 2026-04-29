#include "UiRenderer.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "UiRendererSupport.h"
#include "assets/CaptureHealingLogo.h"

using namespace ui_render;

void UiRenderer::drawLogoPage_() {
  if (logoPageCacheValid_) {
    return;
  }

  const int16_t margin = uiMargin(tft_.width());
  const int16_t gap = uiGap(tft_.width());
  const int16_t headerH = uiHeaderH(tft_.height());
  const int16_t footerH = uiFooterH(tft_.height());
  const int16_t bodyX = margin;
  const int16_t bodyY = margin + headerH + gap;
  const int16_t bodyW = tft_.width() - (2 * margin);
  const int16_t bodyH = tft_.height() - bodyY - footerH - margin - gap;
  const uint16_t cardBg = colorPanelBg();

  const int16_t innerX = bodyX + 8;
  const int16_t innerY = bodyY + 8;
  const int16_t innerW = bodyW - 16;
  const int16_t innerH = bodyH - 16;
  if (innerW > 0 && innerH > 0) {
    tft_.fillRect(innerX, innerY, innerW, innerH, cardBg);
  }

  const int16_t logoW = logo::kCaptureHealingLogoWidth;
  const int16_t logoH = logo::kCaptureHealingLogoHeight;

  const int16_t contentPad = 6;
  const int16_t contentX = innerX + contentPad;
  const int16_t contentY = innerY + contentPad;
  const int16_t contentW = innerW - (2 * contentPad);
  const int16_t contentH = innerH - (2 * contentPad);
  if (contentW <= 32 || contentH <= 32) {
    logoPageCacheValid_ = true;
    return;
  }

  const int16_t splitGap = 8;
  int16_t leftW = 0;
  int16_t rightW = 0;
  if (contentW >= 240) {
    leftW = (contentW * 54) / 100;
    if (leftW < 110) leftW = 110;
    rightW = contentW - leftW - splitGap;
    if (rightW < 110) {
      rightW = 110;
      leftW = contentW - rightW - splitGap;
    }
  } else {
    leftW = (contentW - splitGap) / 2;
    rightW = contentW - leftW - splitGap;
  }

  if (leftW < 48 || rightW < 48) {
    logoPageCacheValid_ = true;
    return;
  }

  const int16_t rightX = contentX + leftW + splitGap;

  const int16_t dividerX = contentX + leftW + (splitGap / 2);
  if (dividerX > contentX && dividerX < (contentX + contentW)) {
    tft_.drawFastVLine(dividerX, contentY + 6, contentH - 12, rgb565(36, 74, 122));
  }

  // Scale logo to fill the left column (fractional scale for smoother sizing).
  int16_t scaledLogoW = leftW - 8;
  int16_t scaledLogoH = static_cast<int16_t>((static_cast<int32_t>(logoH) * scaledLogoW) / logoW);
  if (scaledLogoH > (contentH - 8)) {
    scaledLogoH = contentH - 8;
    scaledLogoW = static_cast<int16_t>((static_cast<int32_t>(logoW) * scaledLogoH) / logoH);
  }
  if (scaledLogoW < logoW) scaledLogoW = logoW;
  if (scaledLogoH < logoH) scaledLogoH = logoH;

  const int16_t logoX = contentX + ((leftW - scaledLogoW) / 2);
  const int16_t logoY = contentY + ((contentH - scaledLogoH) / 2);

  for (int16_t dy = 0; dy < scaledLogoH; ++dy) {
    const int16_t sy = static_cast<int16_t>((static_cast<int32_t>(dy) * logoH) / scaledLogoH);
    for (int16_t dx = 0; dx < scaledLogoW; ++dx) {
      const int16_t sx = static_cast<int16_t>((static_cast<int32_t>(dx) * logoW) / scaledLogoW);
      const uint16_t color = pgm_read_word(&logo::kCaptureHealingLogo565[(sy * logoW) + sx]);
      tft_.drawPixel(logoX + dx, logoY + dy, color);
    }
  }

  const bool compactRight = (rightW < 130);
  tft_.setTextSize(1);
  tft_.setFreeFont(compactRight ? &FreeSansBold9pt7b : &FreeSansBold12pt7b);
  const int16_t titleFontH = static_cast<int16_t>(tft_.fontHeight());
  tft_.setFreeFont(&FreeSansBold9pt7b);
  const int16_t subtitleFontH = static_cast<int16_t>(tft_.fontHeight());
  const int16_t textBlockH = (titleFontH * 2) + 6 + (subtitleFontH * 2);
  int16_t textY = contentY + ((contentH - textBlockH) / 2);
  if (textY < contentY + 4) textY = contentY + 4;

  tft_.setTextDatum(TL_DATUM);
  tft_.setTextColor(rgb565(229, 244, 255), cardBg);
  tft_.setTextSize(1);
  tft_.setFreeFont(compactRight ? &FreeSansBold9pt7b : &FreeSansBold12pt7b);
  tft_.drawString("Capture", rightX + 2, textY);
  tft_.drawString("Healing", rightX + 2, textY + titleFontH);

  tft_.setTextSize(1);
  tft_.setFreeFont(&FreeSansBold9pt7b);
  tft_.setTextColor(rgb565(180, 220, 245), cardBg);
  tft_.drawString("Cold Atmospheric", rightX + 2, textY + (titleFontH * 2) + 6);
  tft_.drawString("Plasma Research", rightX + 2, textY + (titleFontH * 2) + 6 + subtitleFontH);
  applyUiFont(tft_, UiFontRole::Small);
  tft_.setTextDatum(TL_DATUM);

  logoPageCacheValid_ = true;
}

void UiRenderer::drawMetricCards_(const SensorSnapshot& snapshot) {
  drawMetricCardDynamic_(metricCardGeometry_(MetricKind::Temperature), MetricKind::Temperature,
                         snapshot.temperatureC, snapshot.tempHumidityOk);
  drawMetricCardDynamic_(metricCardGeometry_(MetricKind::Humidity), MetricKind::Humidity,
                         snapshot.humidityPct, snapshot.tempHumidityOk);
  drawMetricCardDynamic_(metricCardGeometry_(MetricKind::Distance), MetricKind::Distance,
                         snapshot.distanceMm, snapshot.distanceOk);
}

void UiRenderer::drawTrendsPage_(const SensorSnapshot& snapshot, const TrendBuffer& trends,
                                 bool refreshSparklines) {
  const int16_t margin = uiMargin(tft_.width());
  const int16_t gap = uiGap(tft_.width());
  const int16_t headerH = uiHeaderH(tft_.height());
  const int16_t footerH = uiFooterH(tft_.height());
  const int16_t bodyX = margin;
  const int16_t bodyY = margin + headerH + gap;
  const int16_t bodyW = tft_.width() - (2 * margin);
  const int16_t bodyH = tft_.height() - bodyY - footerH - margin - gap;
  const uint16_t cardBg = colorPanelBg();

  const int16_t chartX = bodyX + 10;
  const int16_t chartY = bodyY + 10;
  const int16_t chartW = bodyW - 20;
  const int16_t chartH = bodyH - 50;

  if ((refreshSparklines || !trendsCache_.initialized) && chartW > 6 && chartH > 6) {
    drawCombinedTrendsChart_(chartX, chartY, chartW, chartH, trends);
  }

  char tempText[24];
  char humidityText[24];
  char distanceText[24];
  formatMetricValue_(MetricKind::Temperature, snapshot.temperatureC, snapshot.tempHumidityOk,
                     tempText, sizeof(tempText));
  formatMetricValue_(MetricKind::Humidity, snapshot.humidityPct, snapshot.tempHumidityOk,
                     humidityText, sizeof(humidityText));
  formatMetricValue_(MetricKind::Distance, snapshot.distanceMm, snapshot.distanceOk, distanceText,
                     sizeof(distanceText));
  const size_t sampleCount = trends.count();

  const bool changed = !trendsCache_.initialized ||
                       (trendsCache_.tempHumidityOk != snapshot.tempHumidityOk) ||
                       (trendsCache_.distanceOk != snapshot.distanceOk) ||
                       (strcmp(trendsCache_.tempText, tempText) != 0) ||
                       (strcmp(trendsCache_.humidityText, humidityText) != 0) ||
                       (strcmp(trendsCache_.distanceText, distanceText) != 0) ||
                       (trendsCache_.sampleCount != sampleCount);

  if (!changed) {
    return;
  }

  const int16_t statsX = bodyX + 10;
  const int16_t statsY = chartY + chartH + 4;
  const int16_t statsW = bodyW - 20;
  const int16_t statsH = bodyY + bodyH - statsY - 8;
  if (statsW > 0 && statsH > 0) {
    tft_.fillRect(statsX, statsY, statsW, statsH, cardBg);
  }

  if (statsW > 30 && statsH > 10) {
    const int16_t colW = statsW / 3;
    const int16_t c1 = statsX + (colW / 2);
    const int16_t c2 = statsX + colW + (colW / 2);
    const int16_t c3 = statsX + (2 * colW) + ((statsW - (2 * colW)) / 2);
    const int16_t valueY = statsY + (statsH / 2);

    char tempValue[24];
    char humidityValue[24];
    char distanceValue[24];
    snprintf(tempValue, sizeof(tempValue), "T %sC", tempText);
    snprintf(humidityValue, sizeof(humidityValue), "RH %s%%", humidityText);
    snprintf(distanceValue, sizeof(distanceValue), "D %smm", distanceText);

    tft_.setTextDatum(MC_DATUM);
    applyUiFont(tft_, UiFontRole::Medium);
    tft_.setTextColor(colorTextStrong(), cardBg);
    tft_.drawString(tempValue, c1, valueY);
    tft_.drawString(humidityValue, c2, valueY);
    tft_.drawString(distanceValue, c3, valueY);
    applyUiFont(tft_, UiFontRole::Small);
  }

  trendsCache_.initialized = true;
  trendsCache_.tempHumidityOk = snapshot.tempHumidityOk;
  trendsCache_.distanceOk = snapshot.distanceOk;
  trendsCache_.sampleCount = sampleCount;
  strncpy(trendsCache_.tempText, tempText, sizeof(trendsCache_.tempText) - 1);
  trendsCache_.tempText[sizeof(trendsCache_.tempText) - 1] = '\0';
  strncpy(trendsCache_.humidityText, humidityText, sizeof(trendsCache_.humidityText) - 1);
  trendsCache_.humidityText[sizeof(trendsCache_.humidityText) - 1] = '\0';
  strncpy(trendsCache_.distanceText, distanceText, sizeof(trendsCache_.distanceText) - 1);
  trendsCache_.distanceText[sizeof(trendsCache_.distanceText) - 1] = '\0';
}

void UiRenderer::drawLoggingPage_(uint32_t nowMs, const SensorSnapshot& snapshot,
                                  const TrendBuffer& trends) {
  const int16_t margin = uiMargin(tft_.width());
  const int16_t gap = uiGap(tft_.width());
  const int16_t headerH = uiHeaderH(tft_.height());
  const int16_t footerH = uiFooterH(tft_.height());
  const int16_t bodyX = margin;
  const int16_t bodyY = margin + headerH + gap;
  const int16_t bodyW = tft_.width() - (2 * margin);
  const int16_t bodyH = tft_.height() - bodyY - footerH - margin - gap;
  const uint16_t cardBg = colorPanelBg();

  (void)snapshot;
  char fileName[32];
  snprintf(fileName, sizeof(fileName), "env_%08lu.csv",
           static_cast<unsigned long>((nowMs / 86400000UL) + 1UL));
  auto formatElapsed = [](uint32_t totalSeconds, char* out, size_t outSize) {
    const uint32_t hours = totalSeconds / 3600UL;
    const uint32_t minutes = (totalSeconds % 3600UL) / 60UL;
    const uint32_t seconds = totalSeconds % 60UL;
    if (hours > 0) {
      snprintf(out, outSize, "%lu:%02lu:%02lu", static_cast<unsigned long>(hours),
               static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
    } else {
      snprintf(out, outSize, "%02lu:%02lu", static_cast<unsigned long>(minutes),
               static_cast<unsigned long>(seconds));
    }
  };

  const size_t trendCount = trends.count();
  const size_t recentCount = (trendCount < 5U) ? trendCount : 5U;
  char recentRows[5][72] = {};
  for (size_t i = 0; i < recentCount; ++i) {
    const TrendPoint p = trends.atOldest(trendCount - 1U - i);
    const uint32_t timeSec = p.sampleMs / 1000UL;
    char t[16];
    char h[16];
    char d[16];
    char elapsed[16];
    formatElapsed(timeSec, elapsed, sizeof(elapsed));
    formatMetricValue_(MetricKind::Temperature, p.temperatureC, validNumber(p.temperatureC), t,
                       sizeof(t));
    formatMetricValue_(MetricKind::Humidity, p.humidityPct, validNumber(p.humidityPct), h,
                       sizeof(h));
    formatMetricValue_(MetricKind::Distance, p.distanceMm, validNumber(p.distanceMm), d,
                       sizeof(d));
    snprintf(recentRows[i], sizeof(recentRows[i]), "%s|%s|%s|%s", elapsed, t, h, d);
  }

  bool changed = !loggingCache_.initialized ||
                 (strcmp(loggingCache_.fileName, fileName) != 0) ||
                 (loggingCache_.recentCount != recentCount);
  if (!changed) {
    for (size_t i = 0; i < 5U; ++i) {
      if (strcmp(loggingCache_.recentRows[i], recentRows[i]) != 0) {
        changed = true;
        break;
      }
    }
  }

  if (!changed) {
    return;
  }

  const bool layoutChanged =
      !loggingCache_.initialized || (strcmp(loggingCache_.fileName, fileName) != 0);

  const int16_t infoX = bodyX + 8;
  const int16_t infoY = bodyY + 8;
  const int16_t infoW = bodyW - 16;
  const int16_t infoH = bodyH - 16;
  if (layoutChanged && infoW > 0 && infoH > 0) {
    tft_.fillRect(infoX, infoY, infoW, infoH, cardBg);
  }

  const int16_t tableX = infoX + 2;
  const int16_t tableY = infoY + 20;
  const int16_t tableW = infoW - 4;
  const int16_t tableRows = 5;
  int16_t rowH = (bodyY + bodyH - tableY - 8) / (1 + tableRows);
  if (rowH > 22) rowH = 22;
  if (rowH < 18) rowH = 18;
  const int16_t tableH = (rowH * (1 + tableRows)) + 2;
  const uint16_t tableBg = rgb565(16, 28, 44);
  const uint16_t headerBg = rgb565(32, 50, 73);
  const uint16_t grid = rgb565(66, 92, 122);
  const uint16_t headerText = rgb565(236, 244, 252);
  const uint16_t cellText = rgb565(222, 233, 244);
  const uint16_t mutedText = rgb565(148, 169, 191);
  const uint16_t zebra = rgb565(21, 35, 53);
  const uint16_t fileText = rgb565(247, 250, 253);

  tft_.setTextDatum(TL_DATUM);
  tft_.setTextSize(1);
  tft_.setFreeFont(&FreeSansBold9pt7b);
  tft_.setTextColor(fileText, cardBg);
  if (layoutChanged) {
    tft_.fillRect(infoX + 1, infoY + 1, infoW - 2, 20, cardBg);
  }
  char fileLine[64];
  snprintf(fileLine, sizeof(fileLine), "Log file: %s", fileName);
  tft_.drawString(fileLine, infoX + 2, infoY + 3);

  const int16_t col1W = (tableW * 24) / 100;  // Elapsed
  const int16_t col2W = (tableW * 20) / 100;  // Temp
  const int16_t col3W = (tableW * 28) / 100;  // Humidity
  const int16_t col4W = tableW - col1W - col2W - col3W;  // Distance
  const int16_t c1CenterX = tableX + (col1W / 2);
  const int16_t col1X = tableX + col1W;
  const int16_t c2CenterX = col1X + (col2W / 2);
  const int16_t col2X = col1X + col2W;
  const int16_t c3CenterX = col2X + (col3W / 2);
  const int16_t col3X = col2X + col3W;
  const int16_t c4CenterX = col3X + (col4W / 2);
  (void)col4W;

  if (layoutChanged) {
    tft_.fillRoundRect(tableX, tableY, tableW, tableH, 6, tableBg);
    tft_.drawRoundRect(tableX, tableY, tableW, tableH, 6, grid);
    tft_.fillRect(tableX + 1, tableY + 1, tableW - 2, rowH, headerBg);

    tft_.drawFastVLine(col1X, tableY + 1, tableH - 2, grid);
    tft_.drawFastVLine(col2X, tableY + 1, tableH - 2, grid);
    tft_.drawFastVLine(col3X, tableY + 1, tableH - 2, grid);
    for (int16_t r = 1; r <= tableRows; ++r) {
      const int16_t y = tableY + (r * rowH);
      tft_.drawFastHLine(tableX + 1, y, tableW - 2, grid);
    }

    const int16_t headerCenterY = tableY + (rowH / 2);
    tft_.setTextSize(1);
    tft_.setFreeFont(&FreeSansBold9pt7b);
    tft_.setTextDatum(MC_DATUM);
    tft_.setTextColor(headerText, headerBg);
    tft_.drawString("Elapsed", c1CenterX, headerCenterY);
    tft_.drawString("Temp C", c2CenterX, headerCenterY);
    tft_.drawString("Humidity %", c3CenterX, headerCenterY);
    tft_.drawString("Dist mm", c4CenterX, headerCenterY);
  }

  tft_.setTextSize(1);
  tft_.setFreeFont(&FreeSansBold9pt7b);
  tft_.setTextDatum(MC_DATUM);
  for (size_t i = 0; i < static_cast<size_t>(tableRows); ++i) {
    const bool zebraRow = ((i + 1U) % 2U) == 0U;
    const bool newestRow = (i == 0U);
    const uint16_t rowBg = newestRow ? rgb565(30, 48, 69) : (zebraRow ? zebra : tableBg);
    const int16_t rowTop = tableY + rowH + static_cast<int16_t>(i * rowH);
    tft_.fillRect(tableX + 1, rowTop + 1, col1W - 1, rowH - 1, rowBg);
    tft_.fillRect(col1X + 1, rowTop + 1, col2W - 1, rowH - 1, rowBg);
    tft_.fillRect(col2X + 1, rowTop + 1, col3W - 1, rowH - 1, rowBg);
    tft_.fillRect(col3X + 1, rowTop + 1, col4W - 2, rowH - 1, rowBg);

    const int16_t rowCenterY =
        tableY + rowH + (rowH / 2) + static_cast<int16_t>(i * rowH);
    if (i < recentCount) {
      const TrendPoint p = trends.atOldest(trendCount - 1U - i);
      const uint32_t timeSec = p.sampleMs / 1000UL;
      char t[16];
      char h[16];
      char d[16];
      char elapsed[16];
      formatElapsed(timeSec, elapsed, sizeof(elapsed));
      formatMetricValue_(MetricKind::Temperature, p.temperatureC, validNumber(p.temperatureC), t,
                         sizeof(t));
      formatMetricValue_(MetricKind::Humidity, p.humidityPct, validNumber(p.humidityPct), h,
                         sizeof(h));
      formatMetricValue_(MetricKind::Distance, p.distanceMm, validNumber(p.distanceMm), d,
                         sizeof(d));
      tft_.setTextColor(newestRow ? colorTextStrong() : cellText, rowBg);
      tft_.drawString(elapsed, c1CenterX, rowCenterY);
      tft_.drawString(t, c2CenterX, rowCenterY);
      tft_.drawString(h, c3CenterX, rowCenterY);
      tft_.drawString(d, c4CenterX, rowCenterY);
    } else {
      tft_.setTextColor(mutedText, rowBg);
      tft_.drawString("-", c1CenterX, rowCenterY);
      tft_.drawString("-", c2CenterX, rowCenterY);
      tft_.drawString("-", c3CenterX, rowCenterY);
      tft_.drawString("-", c4CenterX, rowCenterY);
    }
  }
  applyUiFont(tft_, UiFontRole::Small);
  tft_.setTextDatum(TL_DATUM);

  loggingCache_.initialized = true;
  strncpy(loggingCache_.fileName, fileName, sizeof(loggingCache_.fileName) - 1);
  loggingCache_.fileName[sizeof(loggingCache_.fileName) - 1] = '\0';
  loggingCache_.recentCount = recentCount;
  for (size_t i = 0; i < 5U; ++i) {
    strncpy(loggingCache_.recentRows[i], recentRows[i], sizeof(loggingCache_.recentRows[i]) - 1);
    loggingCache_.recentRows[i][sizeof(loggingCache_.recentRows[i]) - 1] = '\0';
  }
}
