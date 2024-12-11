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
      const std::shared_ptr<PeriodicWave> &periodicWave);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;
};
} // namespace audioapi
