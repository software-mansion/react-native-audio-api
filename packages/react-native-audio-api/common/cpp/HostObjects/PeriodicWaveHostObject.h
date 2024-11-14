#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include "PeriodicWaveWrapper.h"

namespace audioapi {
using namespace facebook;

class PeriodicWaveHostObject : public jsi::HostObject {
 public:
  std::shared_ptr<PeriodicWaveWrapper> wrapper_;

  explicit PeriodicWaveHostObject(
      const std::shared_ptr<PeriodicWaveWrapper> &wrapper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  static std::shared_ptr<PeriodicWaveHostObject> createFromWrapper(
      const std::shared_ptr<PeriodicWaveWrapper> &wrapper) {
    return std::make_shared<PeriodicWaveHostObject>(wrapper);
  }
};
} // namespace audioapi
