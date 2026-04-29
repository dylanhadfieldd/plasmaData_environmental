#pragma once

#include <math.h>

namespace app {

inline bool isFiniteNumber(float value) {
  return !isnan(value) && !isinf(value);
}

}  // namespace app
