#include <audioapi/HostObjects/AudioContextHostObject.h>

#include <audioapi/HostObjects/sources/AudioFileSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/MediaElementAudioSourceNodeHostObject.h>
#include <audioapi/core/AudioContext.h>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AudioContextHostObject::AudioContextHostObject(
    float sampleRate,
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const RuntimeRegistry &runtimeRegistry,
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker)
    : BaseAudioContextHostObject(
          std::make_shared<AudioContext>(sampleRate, audioEventHandlerRegistry, runtimeRegistry),
          runtime,
          callInvoker) {
  addFunctions(
      JSI_EXPORT_FUNCTION(AudioContextHostObject, close),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, suspend),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, createMediaElementSource));
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, close) {
  context_->getGraph()->collectDisposedNodes();
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
  context_->getGraph()->collectDisposedNodes();
  auto audioContext = std::static_pointer_cast<AudioContext>(context_);
  auto promise = promiseVendor_->createAsyncPromise([audioContext = std::move(audioContext)]() {
    const auto result = audioContext->resume();
    return [result](jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
      if (result) {
        return jsi::Value::undefined();
      }
      return std::string("Failed to resume audio context.");
    };
  });
  return promise;
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, suspend) {
  context_->getGraph()->collectDisposedNodes();
  auto audioContext = std::static_pointer_cast<AudioContext>(context_);
  auto promise = promiseVendor_->createAsyncPromise([audioContext = std::move(audioContext)]() {
    const auto result = audioContext->suspend();
    return [result](jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
      if (result) {
        return jsi::Value::undefined();
      }
      return std::string("Failed to suspend audio context.");
    };
  });

  return promise;
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, createMediaElementSource) {
  auto sourceObject = args[0].asObject(runtime);
  auto fileSourceHostObject = sourceObject.getHostObject<AudioFileSourceNodeHostObject>(runtime);
  auto *fileSourceRaw = fileSourceHostObject->audioFileSourceNode();
  auto mediaElementHostObject = std::make_shared<MediaElementAudioSourceNodeHostObject>(
      std::static_pointer_cast<AudioContext>(context_), fileSourceRaw);
  auto object = jsi::Object::createFromHostObject(runtime, mediaElementHostObject);
  object.setExternalMemoryPressure(runtime, mediaElementHostObject->getMemoryPressure());
  return object;
}

} // namespace audioapi
