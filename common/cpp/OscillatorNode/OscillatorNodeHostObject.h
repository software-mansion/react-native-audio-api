#pragma once

#include <memory>
#include <string>
#include <vector>
#include "AudioScheduledSourceNodeHostObject.h"
#include "AudioParamHostObject.h"
#include "OscillatorNodeWrapper.h"

namespace audioapi {
using namespace facebook;

class OscillatorNodeWrapper;

class OscillatorNodeHostObject : public AudioScheduledSourceNodeHostObject {
 private:
  std::shared_ptr<OscillatorNodeWrapper>
  getOscillatorNodeWrapperFromAudioNodeWrapper();

 protected:
  std::shared_ptr<AudioParamHostObject> frequencyParam_;
  std::shared_ptr<AudioParamHostObject> detuneParam_;

 public:
  explicit OscillatorNodeHostObject(
      const std::shared_ptr<OscillatorNodeWrapper> &wrapper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;
  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;
  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  static std::shared_ptr<OscillatorNodeHostObject> createFromWrapper(
      const std::shared_ptr<OscillatorNodeWrapper> &wrapper) {
    return std::make_shared<OscillatorNodeHostObject>(wrapper);
  }
};
} // namespace audioapi
