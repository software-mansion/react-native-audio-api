#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

#include "AudioBufferHostObject.h"
#include "AudioBufferSourceNodeHostObject.h"
#include "AudioDestinationNodeHostObject.h"
#include "BaseAudioContextWrapper.h"
#include "BiquadFilterNodeHostObject.h"
#include "GainNodeHostObject.h"
#include "JsiPromise.h"
#include "OscillatorNodeHostObject.h"
#include "PeriodicWaveHostObject.h"
#include "StereoPannerNodeHostObject.h"

namespace audioapi {
using namespace facebook;

class BaseAudioContextHostObject : public jsi::HostObject {
 public:
  explicit BaseAudioContextHostObject(
      const std::shared_ptr<BaseAudioContextWrapper> &wrapper,
      std::shared_ptr<JsiPromise::PromiseVendor> promiseVendor);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

 protected:
  std::shared_ptr<BaseAudioContextWrapper> wrapper_;
  std::shared_ptr<AudioDestinationNodeHostObject> destination_;
  std::shared_ptr<JsiPromise::PromiseVendor> promiseVendor_;
};
} // namespace audioapi
