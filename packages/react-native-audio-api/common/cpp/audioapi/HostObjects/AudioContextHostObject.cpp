#include <audioapi/HostObjects/AudioContextHostObject.h>

#include <audioapi/core/AudioContext.h>
#include <memory>
#include <utility>

namespace audioapi {

AudioContextHostObject::AudioContextHostObject(
    const std::shared_ptr<AudioContext> &audioContext,
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker)
    : BaseAudioContextHostObject(audioContext, runtime, callInvoker) {
  addFunctions(
      JSI_EXPORT_FUNCTION(AudioContextHostObject, close),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, suspend));
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, close) {
  auto audioContext = std::static_pointer_cast<AudioContext>(context_);
  auto promise = promiseVendor_->createAsyncPromise([audioContext = std::move(audioContext)]() {
    return [audioContext](jsi::Runtime &runtime) {
      audioContext->close();
      return jsi::Value::undefined();
    };
  });

  return promise;
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, resume) {
  auto audioContext = std::static_pointer_cast<AudioContext>(context_);
  auto promise = promiseVendor_->createAsyncPromise([audioContext = std::move(audioContext)]() {
    auto result = audioContext->resume();
    return [result](jsi::Runtime &runtime) {
      return jsi::Value(result);
    };
  });
  return promise;
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, suspend) {
  auto audioContext = std::static_pointer_cast<AudioContext>(context_);
  auto promise = promiseVendor_->createAsyncPromise([audioContext = std::move(audioContext)]() {
    auto result = audioContext->suspend();
    return [result](jsi::Runtime &runtime) {
      return jsi::Value(result);
    };
  });

  return promise;
}

} // namespace audioapi
