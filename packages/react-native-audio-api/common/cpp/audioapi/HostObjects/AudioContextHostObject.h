#pragma once

#include <audioapi/core/AudioContext.h>
#include <audioapi/HostObjects/BaseAudioContextHostObject.h>

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioContextHostObject : public BaseAudioContextHostObject {
 public:
  explicit AudioContextHostObject(
      const std::shared_ptr<AudioContext> &audioContext,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker)
      : BaseAudioContextHostObject(audioContext, runtime, callInvoker) {
    addFunctions(
      JSI_EXPORT_FUNCTION(AudioContextHostObject, close),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, suspend));
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

  JSI_HOST_FUNCTION(resume) {
    auto promise = promiseVendor_->createPromise([this](std::shared_ptr<Promise> promise) {
      std::thread([this, promise = std::move(promise)]() {
          auto audioContext = std::static_pointer_cast<AudioContext>(context_);
          auto result = audioContext->resume();

          promise->resolve([result](jsi::Runtime &runtime) {
            return jsi::Value(result);
          });
      }).detach();
    });

    return promise;
  }

  JSI_HOST_FUNCTION(suspend) {
    auto promise = promiseVendor_->createPromise([this](std::shared_ptr<Promise> promise) {
      std::thread([this, promise = std::move(promise)]() {
          auto audioContext = std::static_pointer_cast<AudioContext>(context_);
          auto result = audioContext->suspend();

          promise->resolve([result](jsi::Runtime &runtime) {
            return jsi::Value(result);
          });
      }).detach();
    });

    return promise;
  }
};
} // namespace audioapi
