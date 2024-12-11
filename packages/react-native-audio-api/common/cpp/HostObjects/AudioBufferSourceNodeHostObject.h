#pragma once

#include <memory>
#include <vector>

#include "AudioBufferHostObject.h"
#include "AudioBufferSourceNode.h"
#include "AudioScheduledSourceNodeHostObject.h"

namespace audioapi {
using namespace facebook;

class AudioBufferSourceNodeHostObject
    : public AudioScheduledSourceNodeHostObject {
 public:
  explicit AudioBufferSourceNodeHostObject(
      const std::shared_ptr<AudioBufferSourceNode> &node);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

 private:
  std::shared_ptr<AudioBufferSourceNode>
  getAudioBufferSourceNodeFromAudioNode();
};
} // namespace audioapi
