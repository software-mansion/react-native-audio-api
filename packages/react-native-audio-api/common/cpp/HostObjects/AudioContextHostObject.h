#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

#include "AudioContext.h"
#include "BaseAudioContextHostObject.h"

namespace audioapi {
using namespace facebook;

class AudioContextHostObject : public BaseAudioContextHostObject {
 public:
  explicit AudioContextHostObject(
      const std::shared_ptr<AudioContext> &audioContext,
      const std::shared_ptr<PromiseVendor> &promiseVendor)
      : BaseAudioContextHostObject(audioContext, promiseVendor) {
    addFunctions(JSI_EXPORT_FUNCTION(AudioContextHostObject, close));
  }

  JSI_HOST_FUNCTION(close) {
    auto promise = promiseVendor_->createPromise([this](std::shared_ptr<Promise> promise) {
      std::thread([this, promise = std::move(promise)]() {
          auto audioContext = std::static_pointer_cast<AudioContext>(context_);
          audioContext->close();

          promise->resolve([](jsi::Runtime &runtime) {
              return jsi::Value::undefined();
          });
      }).detach();
    });

    return promise;
  }
};
} // namespace audioapi
