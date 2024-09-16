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

class AudioContextWrapper;

class AudioContextHostObject : public jsi::HostObject {
 private:
  std::shared_ptr<AudioContextWrapper> wrapper_;
  std::shared_ptr<AudioDestinationNodeHostObject> destinationNode_;

 public:
  explicit AudioContextHostObject(
      const std::shared_ptr<AudioContextWrapper> &wrapper);

#ifdef ANDROID
  static void createAndInstallFromWrapper(
      const std::shared_ptr<AudioContextWrapper> &wrapper,
      jlong jsContext) {
    auto runtime = reinterpret_cast<jsi::Runtime *>(jsContext);
    auto hostObject = std::make_shared<AudioContextHostObject>(wrapper);
    auto object = jsi::Object::createFromHostObject(*runtime, hostObject);
    runtime->global().setProperty(
        *runtime, "__AudioContext", std::move(object));
  }
#endif

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;
  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;
  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;
};
} // namespace audioapi
