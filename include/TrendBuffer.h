#pragma once

#include <stddef.h>
#include <math.h>

#include "AppTypes.h"

class TrendBuffer {
 public:
  static constexpr size_t kCapacity = 180;  // 3 minutes @ 1 Hz

  void push(float temperatureC, float humidityPct, float distanceMm) {
    data_[head_].temperatureC = temperatureC;
    data_[head_].humidityPct = humidityPct;
    data_[head_].distanceMm = distanceMm;

    head_ = (head_ + 1) % kCapacity;
    if (count_ < kCapacity) {
      ++count_;
    }
  }

  size_t count() const { return count_; }

  TrendPoint atOldest(size_t index) const {
    if (index >= count_) {
      return TrendPoint{};
    }

    size_t oldest = (head_ + kCapacity - count_) % kCapacity;
    size_t realIndex = (oldest + index) % kCapacity;
    return data_[realIndex];
  }

 private:
  TrendPoint data_[kCapacity]{};
  size_t head_ = 0;   // next write position
  size_t count_ = 0;  // number of valid points
};

