#pragma once

#include <memory>
#include <vector>

#include "AudioBufferHostObject.h"
#include "AudioBufferSourceNodeWrapper.h"
#include "AudioScheduledSourceNodeHostObject.h"

namespace audioapi {
using namespace facebook;

class AudioBufferSourceNodeWrapper;

class AudioBufferSourceNodeHostObject
    : public AudioScheduledSourceNodeHostObject {
 public:
  explicit AudioBufferSourceNodeHostObject(
      const std::shared_ptr<AudioBufferSourceNodeWrapper> &wrapper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  static std::shared_ptr<AudioBufferSourceNodeHostObject> createFromWrapper(
      const std::shared_ptr<AudioBufferSourceNodeWrapper> &wrapper) {
    return std::make_shared<AudioBufferSourceNodeHostObject>(wrapper);
  }

 private:
  std::shared_ptr<AudioBufferSourceNodeWrapper>
  getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
};
} // namespace audioapi
