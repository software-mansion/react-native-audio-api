#pragma once

#include <memory>

#include "PeriodicWave.h"

namespace audioapi {

class PeriodicWaveWrapper {
 public:
  explicit PeriodicWaveWrapper(
      const std::shared_ptr<PeriodicWave> &periodicWave)
      : periodicWave_(periodicWave) {}

  std::shared_ptr<PeriodicWave> periodicWave_;
};
} // namespace audioapi
