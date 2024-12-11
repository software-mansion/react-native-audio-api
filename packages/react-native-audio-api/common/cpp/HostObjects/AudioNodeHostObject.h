#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include "AudioNode.h"

namespace audioapi {
using namespace facebook;

class AudioNodeHostObject : public jsi::HostObject {
 public:
  explicit AudioNodeHostObject(
      const std::shared_ptr<AudioNode> &node);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

 protected:
  std::shared_ptr<AudioNode> node_;
};
} // namespace audioapi
