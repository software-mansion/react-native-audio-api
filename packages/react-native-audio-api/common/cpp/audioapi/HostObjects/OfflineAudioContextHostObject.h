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
      const std::shared_ptr<PromiseVendor> &promiseVendor,
      const std::shared_ptr<react::CallInvoker> &jsCallInvoker)
      : BaseAudioContextHostObject(audioContext, promiseVendor),
        jsCallInvoker_(jsCallInvoker) {
    addFunctions(
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, suspend),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, startRendering));
  }
  
  JSI_HOST_FUNCTION(resume) {
    auto promise = promiseVendor_->createPromise([this](std::shared_ptr<Promise> promise) {
      auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);
      audioContext->resume();
    });

    return promise;
  }

  JSI_HOST_FUNCTION(suspend) {
    double when = args[0].getNumber();

    auto promise = promiseVendor_->createPromise([this, when](std::shared_ptr<Promise> promise) {
      auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);
      OfflineAudioContextSuspendCallback callback = [promise]() {
        promise->resolve([](jsi::Runtime &runtime) {
          return jsi::Value::undefined();
        });
      };
      audioContext->suspend(when, callback);
    });

    return promise;
  }

  JSI_HOST_FUNCTION(startRendering) {
    auto promise = promiseVendor_->createPromise([this](std::shared_ptr<Promise> promise) {
      auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);

      OfflineAudioContextResultCallback callback = 
          [promise](std::shared_ptr<AudioBuffer> audioBuffer) -> void {
        auto audioBufferHostObject = std::make_shared<AudioBufferHostObject>(audioBuffer);
        promise->resolve([audioBufferHostObject = std::move(audioBufferHostObject)](jsi::Runtime &runtime) {
          return jsi::Object::createFromHostObject(runtime, audioBufferHostObject);
        });
      };
      
      audioContext->startRendering(callback);
    });

    return promise;
  }

  protected:
   std::shared_ptr<react::CallInvoker> jsCallInvoker_;
};
} // namespace audioapi
