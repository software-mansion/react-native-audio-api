#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include "AudioContext.h"
#include "BaseAudioContextHostObject.h"

namespace audioapi {
using namespace facebook;

class AudioContextHostObject : public BaseAudioContextHostObject {
 public:
  explicit AudioContextHostObject(
      const std::shared_ptr<AudioContext> &audioContext,
      const std::shared_ptr<JsiPromise::PromiseVendor> &promiseVendor)
      : BaseAudioContextHostObject(audioContext, promiseVendor) {
    addFunctions(JSI_EXPORT_FUNCTION(AudioContextHostObject, close));
  }

  JSI_HOST_FUNCTION(close) {
    auto audioContext = std::static_pointer_cast<AudioContext>(context_);
    audioContext->close();
    return jsi::Value::undefined();
  }
};
} // namespace audioapi
