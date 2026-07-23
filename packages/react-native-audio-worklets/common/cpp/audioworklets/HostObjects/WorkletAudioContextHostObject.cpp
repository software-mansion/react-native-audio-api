#include <audioworklets/HostObjects/WorkletAudioContextHostObject.h>

#include <audioworklets/core/WorkletAudioContext.h>

#include <memory>
#include <string>
#include <utility>

namespace audioworklets {

WorkletAudioContextHostObject::WorkletAudioContextHostObject(
    float sampleRate,
    const std::shared_ptr<audioapi::IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker)
    : audioapi::BaseAudioContextHostObject(
          std::make_shared<WorkletAudioContext>(sampleRate, audioEventHandlerRegistry),
          runtime,
          callInvoker) {
  addFunctions(
      JSI_EXPORT_FUNCTION(WorkletAudioContextHostObject, close),
      JSI_EXPORT_FUNCTION(WorkletAudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(WorkletAudioContextHostObject, suspend));
}

JSI_HOST_FUNCTION_IMPL(WorkletAudioContextHostObject, close) {
  context_->getGraph()->collectDisposedNodes();
  auto audioContext = std::static_pointer_cast<WorkletAudioContext>(context_);
  auto promise = promiseVendor_->createAsyncPromise([audioContext = std::move(audioContext)]() {
    return [audioContext](jsi::Runtime &runtime) {
      audioContext->close();
      return jsi::Value::undefined();
    };
  });

  return promise;
}

JSI_HOST_FUNCTION_IMPL(WorkletAudioContextHostObject, resume) {
  context_->getGraph()->collectDisposedNodes();
  auto audioContext = std::static_pointer_cast<WorkletAudioContext>(context_);
  auto promise = promiseVendor_->createAsyncPromise([audioContext = std::move(audioContext)]() {
    const auto result = audioContext->resume();
    return [result](jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
      if (result) {
        return jsi::Value::undefined();
      }
      return std::string("Failed to resume worklet audio context.");
    };
  });
  return promise;
}

JSI_HOST_FUNCTION_IMPL(WorkletAudioContextHostObject, suspend) {
  context_->getGraph()->collectDisposedNodes();
  auto audioContext = std::static_pointer_cast<WorkletAudioContext>(context_);
  auto promise = promiseVendor_->createAsyncPromise([audioContext = std::move(audioContext)]() {
    const auto result = audioContext->suspend();
    return [result](jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
      if (result) {
        return jsi::Value::undefined();
      }
      return std::string("Failed to suspend worklet audio context.");
    };
  });

  return promise;
}

} // namespace audioworklets
