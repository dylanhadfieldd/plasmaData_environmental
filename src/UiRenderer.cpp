#include "UiRenderer.h"

#include <Wire.h>

#include "UiRendererSupport.h"

using namespace ui_render;

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
    Serial.println(
        F("[UI] Capacitive touch IC not detected; using TFT_eSPI touch fallback if configured."));
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
