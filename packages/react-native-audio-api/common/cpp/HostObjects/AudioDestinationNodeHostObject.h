#pragma once

#include <memory>
#include <vector>

#include "AudioDestinationNode.h"
#include "AudioNodeHostObject.h"

namespace audioapi {
using namespace facebook;

class AudioDestinationNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioDestinationNodeHostObject(
      const std::shared_ptr<AudioDestinationNode> &node);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;
};
} // namespace audioapi
