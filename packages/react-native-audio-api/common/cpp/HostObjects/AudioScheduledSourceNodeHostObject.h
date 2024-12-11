#pragma once

#include <memory>
#include <vector>

#include "AudioNodeHostObject.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {
using namespace facebook;

class AudioScheduledSourceNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioScheduledSourceNodeHostObject(
      const std::shared_ptr<AudioScheduledSourceNode> &node)
      : AudioNodeHostObject(node) {}

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

 private:
  std::shared_ptr<AudioScheduledSourceNode>
  getAudioScheduledSourceNodeFromAudioNode();
};
} // namespace audioapi
