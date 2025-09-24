#include <audioapi/HostObjects/OfflineAudioContextHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/OfflineAudioContext.h>

namespace audioapi {

OfflineAudioContextHostObject::OfflineAudioContextHostObject(
    const std::shared_ptr<OfflineAudioContext> &offlineAudioContext,
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker)
    : BaseAudioContextHostObject(offlineAudioContext, runtime, callInvoker) {
  addFunctions(
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, suspend),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, startRendering));
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, resume) {
  auto promise = promiseVendor_->createPromise(
      [this](const std::shared_ptr<Promise> &promise) {
        auto audioContext =
            std::static_pointer_cast<OfflineAudioContext>(context_);
        audioContext->resume();
      });

  return promise;
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, suspend) {
  double when = args[0].getNumber();

  auto promise = promiseVendor_->createPromise(
      [this, when](const std::shared_ptr<Promise> &promise) {
        auto audioContext =
            std::static_pointer_cast<OfflineAudioContext>(context_);
        OfflineAudioContextSuspendCallback callback = [promise]() {
          promise->resolve(
              [](jsi::Runtime &runtime) { return jsi::Value::undefined(); });
        };
        audioContext->suspend(when, callback);
      });

  return promise;
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, startRendering) {
  auto promise = promiseVendor_->createPromise(
      [this](const std::shared_ptr<Promise> &promise) {
        auto audioContext =
            std::static_pointer_cast<OfflineAudioContext>(context_);

        OfflineAudioContextResultCallback callback =
            [promise](const std::shared_ptr<AudioBuffer> &audioBuffer) -> void {
          auto audioBufferHostObject =
              std::make_shared<AudioBufferHostObject>(audioBuffer);
          promise->resolve([audioBufferHostObject = std::move(
                                audioBufferHostObject)](jsi::Runtime &runtime) {
            return jsi::Object::createFromHostObject(
                runtime, audioBufferHostObject);
          });
        };

        audioContext->startRendering(callback);
      });

  return promise;
}

} // namespace audioapi
