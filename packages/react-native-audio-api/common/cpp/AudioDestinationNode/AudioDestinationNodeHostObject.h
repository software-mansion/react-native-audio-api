#pragma once

#include <memory>
#include <vector>

#include "AudioDestinationNodeWrapper.h"
#include "AudioNodeHostObject.h"

namespace audioapi {
using namespace facebook;

class AudioDestinationNodeWrapper;

class AudioDestinationNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioDestinationNodeHostObject(
      const std::shared_ptr<AudioDestinationNodeWrapper> &wrapper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  static std::shared_ptr<AudioDestinationNodeHostObject> createFromWrapper(
      const std::shared_ptr<AudioDestinationNodeWrapper> &wrapper) {
    return std::make_shared<AudioDestinationNodeHostObject>(wrapper);
  }
};
} // namespace audioapi
