#pragma once

#include <memory>
#include <string>
#include <vector>

#include "BiquadFilterNodeWrapper.h"
#include "AudioNodeHostObject.h"
#include "AudioParamHostObject.h"

namespace audioapi {
using namespace facebook;

class BiquadFilterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit BiquadFilterNodeHostObject(
      const std::shared_ptr<BiquadFilterNodeWrapper> &wrapper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  static std::shared_ptr<BiquadFilterNodeHostObject> createFromWrapper(
      const std::shared_ptr<BiquadFilterNodeWrapper> &wrapper) {
    return std::make_shared<BiquadFilterNodeHostObject>(wrapper);
  }

 private:
  std::shared_ptr<AudioParamHostObject> frequencyParam_;
  std::shared_ptr<AudioParamHostObject> detuneParam_;
  std::shared_ptr<AudioParamHostObject> QParam_;
  std::shared_ptr<AudioParamHostObject> gainParam_;

  std::shared_ptr<BiquadFilterNodeWrapper>
  getBiquadFilterNodeWrapperFromAudioNodeWrapper();
};
} // namespace audioapi
