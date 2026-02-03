#include <audioapi/HostObjects/OfflineAudioContextHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <memory>
#include <utility>

namespace audioapi {

OfflineAudioContextHostObject::OfflineAudioContextHostObject(
        int numberOfChannels,
        size_t length,
        float sampleRate,
        const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
        const RuntimeRegistry &runtimeRegistry,
        jsi::Runtime *runtime,
        const std::shared_ptr<react::CallInvoker> &callInvoker)
    : BaseAudioContextHostObject(std::make_shared<OfflineAudioContext>(numberOfChannels, length, sampleRate, audioEventHandlerRegistry, runtimeRegistry), runtime, callInvoker) {
  addFunctions(
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, suspend),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, startRendering));
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, resume) {
  auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);
  auto promise = promiseVendor_->createAsyncPromise([audioContext]() {
    audioContext->resume();
    return [](jsi::Runtime &runtime) {
      return jsi::Value::undefined();
    };
  });

  return promise;
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, suspend) {
  double when = args[0].getNumber();
  auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);

  auto promise = promiseVendor_->createAsyncPromise([=](Promise &&promise) {
    OfflineAudioContextSuspendCallback callback = [promise = std::move(promise)]() {
      promise.resolve([](jsi::Runtime &runtime) { return jsi::Value::undefined(); });
    };
    audioContext->suspend(when, callback);
  });

  return promise;
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, startRendering) {
  auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);
  auto promise = promiseVendor_->createAsyncPromise([audioContext](Promise &&promise) {
    OfflineAudioContextResultCallback callback =
        [promise = std::move(promise)](const std::shared_ptr<AudioBuffer> &audioBuffer) {
          auto audioBufferHostObject = std::make_shared<AudioBufferHostObject>(audioBuffer);
          promise.resolve([audioBufferHostObject](jsi::Runtime &runtime) {
            return jsi::Object::createFromHostObject(runtime, audioBufferHostObject);
          });
        };

    audioContext->startRendering(callback);
  });

  return promise;
}

} // namespace audioapi
