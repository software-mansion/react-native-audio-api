#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include "PeriodicWave.h"

namespace audioapi {
using namespace facebook;

class PeriodicWaveHostObject : public jsi::HostObject {
 public:
  std::shared_ptr<PeriodicWave> periodicWave_;

  explicit PeriodicWaveHostObject(
      const std::shared_ptr<PeriodicWave> &periodicWave)
      : periodicWave_(periodicWave) {}
};
} // namespace audioapi
