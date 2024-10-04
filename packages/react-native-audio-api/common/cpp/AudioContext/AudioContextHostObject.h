#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

#include "AudioBufferHostObject.h"
#include "AudioBufferSourceNodeHostObject.h"
#include "AudioContextWrapper.h"
#include "AudioDestinationNodeHostObject.h"
#include "BiquadFilterNodeHostObject.h"
#include "GainNodeHostObject.h"
#include "OscillatorNodeHostObject.h"
#include "StereoPannerNodeHostObject.h"

namespace audioapi {
using namespace facebook;

class AudioContextHostObject : public jsi::HostObject {
 public:
  explicit AudioContextHostObject(
      const std::shared_ptr<AudioContextWrapper> &wrapper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  static std::shared_ptr<AudioContextHostObject> createFromWrapper(
      const std::shared_ptr<AudioContextWrapper> &wrapper) {
    return std::make_shared<AudioContextHostObject>(wrapper);
  }

 private:
  std::shared_ptr<AudioContextWrapper> wrapper_;
  std::shared_ptr<AudioDestinationNodeHostObject> destination_;
};
} // namespace audioapi
