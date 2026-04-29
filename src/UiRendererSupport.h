#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "Config.h"
#include "Numeric.h"

namespace ui_render {

inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

inline bool validNumber(float value) {
  return app::isFiniteNumber(value);
}

inline constexpr uint8_t kPageCount = 4;

inline int16_t uiMargin(int16_t width) {
  return (width < 360) ? 10 : 14;
}

inline int16_t uiGap(int16_t width) {
  return (width < 360) ? 8 : 12;
}

inline int16_t uiHeaderH(int16_t height) {
  return (height < 280) ? 50 : 60;
}

inline int16_t uiFooterH(int16_t height) {
  return (height < 280) ? 28 : 32;
}

enum class UiFontRole : uint8_t {
  Small,
  Medium,
  Large,
};

inline void applyUiFont(TFT_eSPI& tft, UiFontRole role) {
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

inline int16_t uiFontHeight(TFT_eSPI& tft, UiFontRole role) {
  applyUiFont(tft, role);
  return static_cast<int16_t>(tft.fontHeight());
}

inline uint16_t colorScreenBg() { return rgb565(8, 18, 34); }
inline uint16_t colorPanelBg() { return rgb565(14, 27, 50); }
inline uint16_t colorPanelAlt() { return rgb565(14, 27, 50); }
inline uint16_t colorPanelBorder() { return rgb565(36, 74, 122); }
inline uint16_t colorHeaderBg() { return rgb565(12, 30, 56); }
inline uint16_t colorHeaderBorder() { return rgb565(56, 118, 182); }
inline uint16_t colorFooterBg() { return rgb565(10, 23, 44); }
inline uint16_t colorTextStrong() { return rgb565(247, 250, 253); }
inline uint16_t colorTextMuted() { return rgb565(151, 193, 223); }
inline uint16_t colorTextSoft() { return rgb565(125, 164, 194); }
inline uint16_t colorGrid() { return rgb565(34, 64, 103); }
inline uint16_t colorChartBg() { return rgb565(9, 21, 39); }
inline uint16_t colorLiveBg() { return rgb565(27, 130, 84); }
inline uint16_t colorLiveText() { return rgb565(233, 252, 239); }
inline uint16_t colorOfflineBg() { return rgb565(136, 38, 43); }
inline uint16_t colorOfflineText() { return rgb565(255, 229, 229); }

}  // namespace ui_render
