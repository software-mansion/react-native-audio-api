#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include "AudioNodeWrapper.h"

namespace audioapi {
using namespace facebook;

class AudioNodeHostObject : public jsi::HostObject {
 public:
  explicit AudioNodeHostObject(
      const std::shared_ptr<AudioNodeWrapper> &wrapper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

 protected:
  std::shared_ptr<AudioNodeWrapper> wrapper_;
};
} // namespace audioapi
