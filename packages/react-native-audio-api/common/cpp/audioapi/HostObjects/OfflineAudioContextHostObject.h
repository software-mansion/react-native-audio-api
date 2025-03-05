#pragma once

#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/HostObjects/BaseAudioContextHostObject.h>

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {
using namespace facebook;

class OfflineAudioContextHostObject : public BaseAudioContextHostObject {
 public:
  explicit OfflineAudioContextHostObject(
      const std::shared_ptr<OfflineAudioContext> &audioContext,
      const std::shared_ptr<PromiseVendor> &promiseVendor)
      : BaseAudioContextHostObject(audioContext, promiseVendor) {
    addFunctions(
      JSI_EXPORT_FUNCTION(AudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, suspend),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, startRendering));
  }
  
  JSI_HOST_FUNCTION(resume) {
    auto promise = promiseVendor_->createPromise([this](std::shared_ptr<Promise> promise) {
      std::thread([this, promise = std::move(promise)]() {
          auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);
          audioContext->resume();

          promise->resolve([](jsi::Runtime &runtime) {
              return jsi::Value::undefined();
          });
      }).detach();
    });

    return promise;
  }

  JSI_HOST_FUNCTION(suspend) {
    auto promise = promiseVendor_->createPromise([this](std::shared_ptr<Promise> promise) {
      std::thread([this, promise = std::move(promise)]() {
          auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);
          audioContext->suspend();

          promise->resolve([](jsi::Runtime &runtime) {
              return jsi::Value::undefined();
          });
      }).detach();
    });

    return promise;
  }

  JSI_HOST_FUNCTION(startRendering) {
    auto promise = promiseVendor_->createPromise([this](std::shared_ptr<Promise> promise) {
      std::thread([this, promise = std::move(promise)]() {
          auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);
          std::shared_ptr<AudioBuffer> audioBuffer = audioContext->startRendering();
          auto audioBufferHostObject = std::make_shared<AudioBufferHostObject>(audioBuffer);

          promise->resolve([audioBufferHostObject = std::move(audioBufferHostObject)](jsi::Runtime &runtime) {
            return jsi::Object::createFromHostObject(runtime, audioBufferHostObject);
          });
      }).detach();
    });

    return promise;
  }
};
} // namespace audioapi
