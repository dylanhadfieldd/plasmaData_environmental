#include "UiRenderer.h"

#include <Wire.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "Config.h"
#include "../CaptureHealingLogo.h"

namespace {

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

bool validNumber(float v) {
  return !isnan(v) && !isinf(v);
}

constexpr uint8_t kPageCount = 4;

int16_t uiMargin(int16_t width) {
  return (width < 360) ? 10 : 14;
}

int16_t uiGap(int16_t width) {
  return (width < 360) ? 8 : 12;
}

int16_t uiHeaderH(int16_t height) {
  return (height < 280) ? 50 : 60;
}

int16_t uiFooterH(int16_t height) {
  return (height < 280) ? 28 : 32;
}

enum class UiFontRole : uint8_t {
  Small,
  Medium,
  Large,
};

void applyUiFont(TFT_eSPI& tft, UiFontRole role) {
  tft.setTextSize(1);
  switch (role) {
    case UiFontRole::Small:
      tft.setFreeFont(nullptr);
      tft.setTextFont(1);
      return;
    case UiFontRole::Medium:
      tft.setFreeFont(nullptr);
      tft.setTextFont(2);
      return;
    case UiFontRole::Large:
      tft.setFreeFont(nullptr);
      tft.setTextFont(2);
      return;
    default:
      tft.setFreeFont(nullptr);
      tft.setTextFont(1);
      return;
  }
}

int16_t uiFontHeight(TFT_eSPI& tft, UiFontRole role) {
  applyUiFont(tft, role);
  return static_cast<int16_t>(tft.fontHeight());
}

constexpr int16_t kLogoScale = 2;

uint16_t colorScreenBg() { return rgb565(8, 18, 34); }
uint16_t colorPanelBg() { return rgb565(14, 27, 50); }
uint16_t colorPanelAlt() { return rgb565(14, 27, 50); }
uint16_t colorPanelBorder() { return rgb565(36, 74, 122); }
uint16_t colorHeaderBg() { return rgb565(12, 30, 56); }
uint16_t colorHeaderBorder() { return rgb565(56, 118, 182); }
uint16_t colorFooterBg() { return rgb565(10, 23, 44); }
uint16_t colorTextStrong() { return rgb565(247, 250, 253); }
uint16_t colorTextMuted() { return rgb565(151, 193, 223); }
uint16_t colorTextSoft() { return rgb565(125, 164, 194); }
uint16_t colorGrid() { return rgb565(34, 64, 103); }
uint16_t colorChartBg() { return rgb565(9, 21, 39); }
uint16_t colorLiveBg() { return rgb565(27, 130, 84); }
uint16_t colorLiveText() { return rgb565(233, 252, 239); }
uint16_t colorOfflineBg() { return rgb565(136, 38, 43); }
uint16_t colorOfflineText() { return rgb565(255, 229, 229); }

}  // namespace

bool UiRenderer::begin() {
  tft_.init();
  tft_.setRotation(cfg::kDisplayRotation);

  pinMode(cfg::kPinTftBacklight, OUTPUT);
  digitalWrite(cfg::kPinTftBacklight, HIGH);

  applyUiFont(tft_, UiFontRole::Small);
  tft_.setTextDatum(TL_DATUM);
  tft_.fillScreen(TFT_BLACK);

  // Optional CTP helper pins. If both are configured (>= 0), run a reset
  // sequence that helps some FT62xx/GT911 modules appear reliably on I2C.
  if (cfg::kPinTouchInt >= 0 && cfg::kPinTouchRst >= 0) {
    pinMode(cfg::kPinTouchInt, OUTPUT);
    pinMode(cfg::kPinTouchRst, OUTPUT);
    digitalWrite(cfg::kPinTouchInt, LOW);
    digitalWrite(cfg::kPinTouchRst, LOW);
    delay(cfg::kTouchResetLowMs);
    digitalWrite(cfg::kPinTouchRst, HIGH);
    delay(cfg::kTouchResetReadyMs);
    pinMode(cfg::kPinTouchInt, INPUT_PULLUP);
    Serial.printf("[UI] Touch pins INT=%d RST=%d initialized.\n", cfg::kPinTouchInt,
                  cfg::kPinTouchRst);
  } else {
    Serial.println(F("[UI] Touch INT/RST helper pins disabled; using I2C-only touch probing."));
  }

  ft62xxTouchReady_ = false;
  const uint8_t ft62xxCandidates[] = {cfg::kTouchI2cAddress, 0x15};
  for (uint8_t addr : ft62xxCandidates) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      ft62xxTouchAddr_ = addr;
      ft62xxTouchReady_ = true;
      break;
    }
  }

  gt911TouchReady_ = false;
  const uint8_t gt911Candidates[] = {0x5D, 0x14};
  for (uint8_t addr : gt911Candidates) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      gt911TouchAddr_ = addr;
      gt911TouchReady_ = true;
      break;
    }
  }

  if (ft62xxTouchReady_) {
    Serial.printf("[UI] FT62xx capacitive touch detected at 0x%02X.\n", ft62xxTouchAddr_);
  }
  if (gt911TouchReady_) {
    Serial.printf("[UI] GT911 capacitive touch detected at 0x%02X.\n", gt911TouchAddr_);
  }
  if (!ft62xxTouchReady_ && !gt911TouchReady_) {
    Serial.println(F("[UI] Capacitive touch IC not detected; using TFT_eSPI touch fallback if configured."));
  }

  Serial.print(F("[UI] I2C addresses seen:"));
  bool anyI2cDevice = false;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf(" 0x%02X", addr);
      anyI2cDevice = true;
    }
  }
  if (!anyI2cDevice) {
    Serial.print(F(" (none)"));
  }
  Serial.println();

  currentPage_ = PageKind::Logo;
  scaffoldDrawn_ = false;
  touchLatched_ = false;
  pendingPageDelta_ = 0;
  lastTouchPollMs_ = 0;
  lastTouchTapMs_ = 0;
  lastSparklineRefreshMs_ = 0;
  resetDynamicCaches_();
  return true;
}

void UiRenderer::resetDynamicCaches_() {
  lastUptimeSecond_ = static_cast<uint32_t>(-1);
  // Force header text redraw after scaffold/page reset.
  lastHeaderPage_ = static_cast<PageKind>(kPageCount);
  footerCacheValid_ = false;
  lastFooterAgeSec_ = static_cast<uint32_t>(-1);
  lastSparklineRefreshMs_ = 0;

  for (MetricRenderCache& cache : metricCache_) {
    cache = MetricRenderCache{};
  }

  trendsCache_ = TrendsPageRenderCache{};
  loggingCache_ = LoggingPageRenderCache{};
  logoPageCacheValid_ = false;
}

void UiRenderer::pollInput(uint32_t nowMs) {
  if (nowMs - lastTouchPollMs_ < cfg::kTouchPollMs) {
    return;
  }
  lastTouchPollMs_ = nowMs;

  if (pendingPageDelta_ != 0) {
    return;
  }
  int8_t pageDelta = 0;
  if (checkForPageTap_(nowMs, pageDelta)) {
    pendingPageDelta_ = pageDelta;
  }
}

void UiRenderer::render(uint32_t nowMs, const SensorSnapshot& snapshot, const TrendBuffer& trends) {
  if (pendingPageDelta_ != 0) {
    if (pendingPageDelta_ > 0) {
      nextPage_();
    } else {
      prevPage_();
    }
    pendingPageDelta_ = 0;
  }

  if (!scaffoldDrawn_) {
    drawScaffold_();
  }

  bool refreshSparklines = false;
  if (lastSparklineRefreshMs_ == 0 ||
      (nowMs - lastSparklineRefreshMs_ >= cfg::kSparklineRefreshMs)) {
    refreshSparklines = true;
    lastSparklineRefreshMs_ = nowMs;
  }

  drawHeader_(nowMs);

  switch (currentPage_) {
    case PageKind::Logo:
      drawLogoPage_();
      break;
    case PageKind::Dashboard:
      drawMetricCards_(snapshot);
      break;
    case PageKind::Trends:
      drawTrendsPage_(snapshot, trends, refreshSparklines);
      break;
    case PageKind::Logging:
      drawLoggingPage_(nowMs, snapshot, trends);
      break;
    default:
      drawMetricCards_(snapshot);
      break;
  }

  drawFooter_(nowMs, snapshot);
}

bool UiRenderer::checkForPageTap_(uint32_t nowMs, int8_t& pageDelta) {
  pageDelta = 0;
  bool down = false;
  uint16_t tx = 0;
  uint16_t ty = 0;
  bool havePosition = false;
  if (!readAnyTouchDown_(down, tx, ty, havePosition)) {
    return false;
  }

  if (down) {
    if (!touchLatched_ && (nowMs - lastTouchTapMs_ >= cfg::kTouchDebounceMs)) {
      touchLatched_ = true;
      lastTouchTapMs_ = nowMs;
      if (havePosition) {
        // Touch controller is mounted with a rotated axis on this build,
        // so use Y to represent left/right paging split.
        pageDelta = (ty < static_cast<uint16_t>(tft_.width() / 2)) ? -1 : 1;
      } else {
        pageDelta = 1;
      }
      return true;
    }
  } else {
    touchLatched_ = false;
  }

  return false;
}

bool UiRenderer::readAnyTouchDown_(bool& down, uint16_t& tx, uint16_t& ty, bool& havePosition) {
  down = false;
  tx = 0;
  ty = 0;
  havePosition = false;

  if (readFt62xxTouchDown_(down, tx, ty, havePosition)) {
    return true;
  }
  if (readGt911TouchDown_(down, tx, ty, havePosition)) {
    return true;
  }

  // If a capacitive controller was detected but this read failed, do not force
  // a "touch release" state from an alternate source on this frame.
  if (ft62xxTouchReady_ || gt911TouchReady_) {
    return false;
  }

#if defined(TOUCH_CS)
  down = tft_.getTouch(&tx, &ty);
  havePosition = down;
  return true;
#else
  down = false;
  return false;
#endif
}

bool UiRenderer::readFt62xxTouchDown_(bool& down, uint16_t& tx, uint16_t& ty, bool& havePosition) {
  down = false;
  havePosition = false;
  if (!ft62xxTouchReady_) {
    return false;
  }

  Wire.beginTransmission(ft62xxTouchAddr_);
  Wire.write(static_cast<uint8_t>(0x02));  // Number of touch points.
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const uint8_t requested = 5;
  const uint8_t received = static_cast<uint8_t>(
      Wire.requestFrom(static_cast<int>(ft62xxTouchAddr_), static_cast<int>(requested)));
  if (received < requested) {
    return false;
  }

  const uint8_t touchPoints = static_cast<uint8_t>(Wire.read()) & 0x0F;
  const uint8_t xh = static_cast<uint8_t>(Wire.read());
  const uint8_t xl = static_cast<uint8_t>(Wire.read());
  const uint8_t yh = static_cast<uint8_t>(Wire.read());
  const uint8_t yl = static_cast<uint8_t>(Wire.read());
  down = (touchPoints > 0);
  if (down) {
    tx = static_cast<uint16_t>(((xh & 0x0FU) << 8) | xl);
    ty = static_cast<uint16_t>(((yh & 0x0FU) << 8) | yl);
    havePosition = true;
  }
  return true;
}

bool UiRenderer::readGt911TouchDown_(bool& down, uint16_t& tx, uint16_t& ty, bool& havePosition) {
  down = false;
  havePosition = false;
  if (!gt911TouchReady_) {
    return false;
  }

  uint8_t status = 0;
  if (!readGt911Register8_(0x814E, status)) {
    return false;
  }

  if ((status & 0x80U) == 0U) {
    down = false;
    return true;
  }

  down = ((status & 0x0FU) > 0U);
  if (down) {
    Wire.beginTransmission(gt911TouchAddr_);
    Wire.write(static_cast<uint8_t>((0x8150 >> 8) & 0xFFU));
    Wire.write(static_cast<uint8_t>(0x8150 & 0xFFU));
    if (Wire.endTransmission(false) == 0) {
      const uint8_t requested = 4;
      const uint8_t received = static_cast<uint8_t>(
          Wire.requestFrom(static_cast<int>(gt911TouchAddr_), static_cast<int>(requested)));
      if (received >= requested) {
        const uint8_t xl = static_cast<uint8_t>(Wire.read());
        const uint8_t xh = static_cast<uint8_t>(Wire.read());
        const uint8_t yl = static_cast<uint8_t>(Wire.read());
        const uint8_t yh = static_cast<uint8_t>(Wire.read());
        tx = static_cast<uint16_t>(xl | (static_cast<uint16_t>(xh) << 8));
        ty = static_cast<uint16_t>(yl | (static_cast<uint16_t>(yh) << 8));
        havePosition = true;
      }
    }
  }
  (void)writeGt911Register8_(0x814E, 0U);
  return true;
}

bool UiRenderer::readGt911Register8_(uint16_t reg, uint8_t& value) {
  value = 0U;
  if (!gt911TouchReady_) {
    return false;
  }

  Wire.beginTransmission(gt911TouchAddr_);
  Wire.write(static_cast<uint8_t>((reg >> 8) & 0xFFU));
  Wire.write(static_cast<uint8_t>(reg & 0xFFU));
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const uint8_t requested = 1;
  const uint8_t received = static_cast<uint8_t>(
      Wire.requestFrom(static_cast<int>(gt911TouchAddr_), static_cast<int>(requested)));
  if (received < requested) {
    return false;
  }

  value = static_cast<uint8_t>(Wire.read());
  return true;
}

bool UiRenderer::writeGt911Register8_(uint16_t reg, uint8_t value) {
  if (!gt911TouchReady_) {
    return false;
  }

  Wire.beginTransmission(gt911TouchAddr_);
  Wire.write(static_cast<uint8_t>((reg >> 8) & 0xFFU));
  Wire.write(static_cast<uint8_t>(reg & 0xFFU));
  Wire.write(value);
  return (Wire.endTransmission() == 0);
}

void UiRenderer::nextPage_() {
  const uint8_t next =
      static_cast<uint8_t>((static_cast<uint8_t>(currentPage_) + 1U) % kPageCount);
  currentPage_ = static_cast<PageKind>(next);
  scaffoldDrawn_ = false;
  resetDynamicCaches_();
}

void UiRenderer::prevPage_() {
  const uint8_t cur = static_cast<uint8_t>(currentPage_);
  const uint8_t prev = static_cast<uint8_t>((cur + kPageCount - 1U) % kPageCount);
  currentPage_ = static_cast<PageKind>(prev);
  scaffoldDrawn_ = false;
  resetDynamicCaches_();
}

const char* UiRenderer::pageTitle_(PageKind page) const {
  switch (page) {
    case PageKind::Logo:
      return "Overview";
    case PageKind::Dashboard:
      return "Live Readings";
    case PageKind::Trends:
      return "Trend Analysis";
    case PageKind::Logging:
      return "Data Logging";
    default:
      return "Overview";
  }
}

const char* UiRenderer::pageSubtitle_(PageKind page) const {
  switch (page) {
    case PageKind::Logo:
      return "Capture Healing monitoring interface";
    case PageKind::Dashboard:
      return "Current environmental sensor values";
    case PageKind::Trends:
      return "Temperature, humidity, and distance history";
    case PageKind::Logging:
      return "Recent rows prepared for CSV export";
    default:
      return "Current environmental sensor values";
  }
}

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
    const int16_t sy = static_cast<int16_t>(
        (static_cast<int32_t>(dy) * logoH) / scaledLogoH);
    for (int16_t dx = 0; dx < scaledLogoW; ++dx) {
      const int16_t sx = static_cast<int16_t>(
          (static_cast<int32_t>(dx) * logoW) / scaledLogoW);
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
  formatMetricValue_(MetricKind::Temperature, snapshot.temperatureC, snapshot.tempHumidityOk, tempText,
                     sizeof(tempText));
  formatMetricValue_(MetricKind::Humidity, snapshot.humidityPct, snapshot.tempHumidityOk, humidityText,
                     sizeof(humidityText));
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

void UiRenderer::drawMetricCardDynamic_(const CardGeometry& geometry, MetricKind metric, float value,
                                        bool valid) {
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
    const int16_t px = plotX + static_cast<int16_t>((static_cast<float>(i) / static_cast<float>(n - 1)) *
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

  snprintf(out, outSize, "Uptime %02lu:%02lu:%02lu", static_cast<unsigned long>(hours),
           static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
}
