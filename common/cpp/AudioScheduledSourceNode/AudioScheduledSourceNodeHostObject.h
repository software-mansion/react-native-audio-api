#pragma once

#include <memory>
#include <vector>

#include "AudioBufferHostObject.h"
#include "AudioNodeHostObject.h"
#include "AudioScheduledSourceNodeWrapper.h"

namespace audioapi {
using namespace facebook;

class AudioScheduledSourceNodeWrapper;

class AudioScheduledSourceNodeHostObject : public AudioNodeHostObject {
 private:
  std::shared_ptr<AudioScheduledSourceNodeWrapper>
  getAudioScheduledSourceNodeWrapperFromAudioNodeWrapper();

 public:
  explicit AudioScheduledSourceNodeHostObject(
      const std::shared_ptr<AudioScheduledSourceNodeWrapper> &wrapper)
      : AudioNodeHostObject(wrapper) {}

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;
  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;
  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;
};
} // namespace audioapi
