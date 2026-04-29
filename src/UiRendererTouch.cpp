#include "UiRenderer.h"

#include <Wire.h>

#include "UiRendererSupport.h"

using namespace ui_render;

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
