#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "AppTypes.h"
#include "TrendBuffer.h"

class UiRenderer {
 public:
  bool begin();
  void pollInput(uint32_t nowMs);
  void render(uint32_t nowMs, const SensorSnapshot& snapshot, const TrendBuffer& trends);

 private:
  enum class PageKind : uint8_t {
    Logo = 0,
    Dashboard = 1,
    Trends = 2,
    Logging = 3,
  };

  enum class MetricKind : uint8_t {
    Temperature,
    Humidity,
    Distance,
  };

  struct CardGeometry {
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = 0;
    int16_t h = 0;
  };

  struct MetricRenderCache {
    bool initialized = false;
    bool valid = false;
    int16_t barFillW = -1;
    char valueText[24] = "";
  };

  struct TrendsPageRenderCache {
    bool initialized = false;
    bool tempHumidityOk = false;
    bool distanceOk = false;
    char tempText[24] = "";
    char humidityText[24] = "";
    char distanceText[24] = "";
    size_t sampleCount = 0;
  };

  struct LoggingPageRenderCache {
    bool initialized = false;
    char fileName[32] = "";
    size_t recentCount = 0;
    char recentRows[5][72] = {};
  };

  TFT_eSPI tft_;
  PageKind currentPage_ = PageKind::Logo;
  bool scaffoldDrawn_ = false;
  uint32_t lastUptimeSecond_ = static_cast<uint32_t>(-1);
  PageKind lastHeaderPage_ = PageKind::Logo;
  bool footerCacheValid_ = false;
  bool lastFooterTempHumidityOk_ = false;
  bool lastFooterDistanceOk_ = false;
  uint32_t lastFooterAgeSec_ = static_cast<uint32_t>(-1);
  uint32_t lastSparklineRefreshMs_ = 0;
  bool touchLatched_ = false;
  int8_t pendingPageDelta_ = 0;
  uint32_t lastTouchPollMs_ = 0;
  uint32_t lastTouchTapMs_ = 0;
  bool ft62xxTouchReady_ = false;
  uint8_t ft62xxTouchAddr_ = 0;
  bool gt911TouchReady_ = false;
  uint8_t gt911TouchAddr_ = 0;
  MetricRenderCache metricCache_[3]{};
  TrendsPageRenderCache trendsCache_{};
  LoggingPageRenderCache loggingCache_{};
  bool logoPageCacheValid_ = false;

  void resetDynamicCaches_();
  bool checkForPageTap_(uint32_t nowMs, int8_t& pageDelta);
  bool readAnyTouchDown_(bool& down, uint16_t& tx, uint16_t& ty, bool& havePosition);
  bool readFt62xxTouchDown_(bool& down, uint16_t& tx, uint16_t& ty, bool& havePosition);
  bool readGt911TouchDown_(bool& down, uint16_t& tx, uint16_t& ty, bool& havePosition);
  bool readGt911Register8_(uint16_t reg, uint8_t& value);
  bool writeGt911Register8_(uint16_t reg, uint8_t value);
  void nextPage_();
  void prevPage_();

  const char* pageTitle_(PageKind page) const;
  const char* pageSubtitle_(PageKind page) const;
  void drawScaffold_();
  void drawPageFrame_();
  void drawVerticalGradient_(int16_t x, int16_t y, int16_t w, int16_t h);
  void drawHeaderFrame_();
  void drawFooterFrame_();
  void drawLogoFrame_();
  void drawMetricCardsFrame_();
  void drawTrendsFrame_();
  void drawLoggingFrame_();
  void drawMetricCardFrame_(const CardGeometry& geometry, MetricKind metric);
  CardGeometry metricCardGeometry_(MetricKind metric);
  int metricIndex_(MetricKind metric) const;
  void drawHeader_(uint32_t nowMs);
  void drawFooter_(uint32_t nowMs, const SensorSnapshot& snapshot);
  void drawLogoPage_();
  void drawMetricCards_(const SensorSnapshot& snapshot);
  void drawTrendsPage_(const SensorSnapshot& snapshot, const TrendBuffer& trends,
                       bool refreshSparklines);
  void drawLoggingPage_(uint32_t nowMs, const SensorSnapshot& snapshot,
                        const TrendBuffer& trends);
  void drawMetricCardDynamic_(const CardGeometry& geometry, MetricKind metric, float value,
                              bool valid);
  void drawStatusBadge_(int16_t x, int16_t y, const char* label, uint16_t fillColor,
                        uint16_t textColor);
  void drawBar_(int16_t x, int16_t y, int16_t w, int16_t h, float normalized,
                uint16_t fillColor);
  void drawCombinedTrendsChart_(int16_t x, int16_t y, int16_t w, int16_t h,
                                const TrendBuffer& trends);

  float pickMetricValue_(const TrendPoint& point, MetricKind metric) const;
  void formatMetricValue_(MetricKind metric, float value, bool valid, char* out,
                          size_t outSize) const;
  float normalize_(float value, float minValue, float maxValue) const;
  void metricMeta_(MetricKind metric, const char*& title, const char*& unit, float& minValue,
                   float& maxValue, uint16_t& accentColor) const;
  void formatUptime_(uint32_t nowMs, char* out, size_t outSize) const;
};
