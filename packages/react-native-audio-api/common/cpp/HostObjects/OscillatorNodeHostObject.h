#pragma once

#include <memory>
#include <string>
#include <vector>

#include "AudioParamHostObject.h"
#include "AudioScheduledSourceNodeHostObject.h"
#include "OscillatorNode.h"
#include "PeriodicWaveHostObject.h"

namespace audioapi {
using namespace facebook;

class OscillatorNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit OscillatorNodeHostObject(
      const std::shared_ptr<OscillatorNode> &node);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

 private:
  std::shared_ptr<AudioParamHostObject> frequencyParam_;
  std::shared_ptr<AudioParamHostObject> detuneParam_;

  std::shared_ptr<OscillatorNode>
  getOscillatorNodeFromAudioNode();
};
} // namespace audioapi
