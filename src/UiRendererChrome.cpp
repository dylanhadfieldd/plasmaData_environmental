#include "UiRenderer.h"

#include <stdio.h>

#include "UiRendererSupport.h"

using namespace ui_render;

void UiRenderer::drawHeader_(uint32_t nowMs) {
  const uint32_t uptimeSeconds = nowMs / 1000UL;
  const bool pageChanged = (currentPage_ != lastHeaderPage_);
  const bool uptimeChanged = (uptimeSeconds != lastUptimeSecond_);
  if (!pageChanged && !uptimeChanged) {
    return;
  }

  const int16_t margin = uiMargin(tft_.width());
  const uint16_t headerBg = colorHeaderBg();
  const int16_t textRight = tft_.width() - margin - 10;

  if (pageChanged) {
    const int16_t headerH = uiHeaderH(tft_.height());
    const int16_t innerX = margin + 2;
    const int16_t innerY = margin + 2;
    const int16_t innerW = tft_.width() - (2 * margin) - 4;
    const int16_t innerH = headerH - 4;
    tft_.fillRect(innerX, innerY, innerW, innerH, headerBg);

    char pageTag[16];
    snprintf(pageTag, sizeof(pageTag), "Page %u/%u",
             static_cast<unsigned>(static_cast<uint8_t>(currentPage_) + 1U),
             static_cast<unsigned>(kPageCount));

    tft_.setTextDatum(TL_DATUM);
    tft_.setTextSize(1);
    tft_.setFreeFont(&FreeSansBold9pt7b);
    tft_.setTextColor(rgb565(220, 243, 255), headerBg);
    tft_.drawString("Capture Healing Lab", margin + 10, margin + 8);
    tft_.setTextColor(rgb565(168, 214, 238), headerBg);
    tft_.drawString(pageTitle_(currentPage_), margin + 10, margin + 32);

    tft_.setTextDatum(TR_DATUM);
    tft_.setTextColor(rgb565(168, 214, 238), headerBg);
    tft_.drawString(pageTag, textRight, margin + 32);
    applyUiFont(tft_, UiFontRole::Small);
  }

  if (pageChanged || uptimeChanged) {
    char uptime[20];
    formatUptime_(nowMs, uptime, sizeof(uptime));
    tft_.setTextSize(1);
    tft_.setFreeFont(&FreeSansBold9pt7b);
    const int16_t clearW = 136;
    const int16_t clearX = textRight - clearW;
    const int16_t clearY = margin + 7;
    const int16_t clearH = static_cast<int16_t>(tft_.fontHeight()) + 2;
    tft_.fillRect(clearX, clearY, clearW, clearH, headerBg);
    tft_.setTextDatum(TR_DATUM);
    tft_.setTextColor(rgb565(255, 252, 237), headerBg);
    tft_.drawString(uptime, textRight, margin + 9);
    applyUiFont(tft_, UiFontRole::Small);
  }

  lastUptimeSecond_ = uptimeSeconds;
  lastHeaderPage_ = currentPage_;
  tft_.setTextDatum(TL_DATUM);
}

void UiRenderer::drawFooter_(uint32_t nowMs, const SensorSnapshot& snapshot) {
  (void)nowMs;
  (void)snapshot;

  if (footerCacheValid_) {
    return;
  }

  footerCacheValid_ = true;

  const int16_t margin = uiMargin(tft_.width());
  const int16_t footerH = uiFooterH(tft_.height());
  const int16_t y = tft_.height() - margin - footerH;
  const uint16_t footerBg = colorFooterBg();
  const char* line = "Sensors Online --- Tap left/right to switch page";
  const int16_t centerX = tft_.width() / 2;
  const int16_t centerY = y + (footerH / 2);

  tft_.fillRect(margin + 4, y + 3, tft_.width() - (2 * margin) - 8, footerH - 6, footerBg);
  tft_.setTextColor(rgb565(196, 225, 248), footerBg);
  tft_.setTextSize(1);
  tft_.setFreeFont(&FreeSansBold9pt7b);
  tft_.setTextDatum(MC_DATUM);
  tft_.drawString(line, centerX, centerY);
  applyUiFont(tft_, UiFontRole::Small);
}
