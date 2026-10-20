#include <audioapi/HostObjects/AudioContextHostObject.h>

#include <audioapi/HostObjects/sources/AudioFileSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/MediaElementAudioSourceNodeHostObject.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/types/ContextState.h>
#include <memory>
#include <utility>

namespace audioapi {

AudioContextHostObject::AudioContextHostObject(
    float sampleRate,
    AndroidOutputProfile androidOutputProfile,
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker,
    AudioContextLatencyHint latencyHint)
    : BaseAudioContextHostObject(
          std::make_shared<AudioContext>(
              sampleRate,
              androidOutputProfile,
              audioEventHandlerRegistry,
              latencyHint),
          runtime,
          callInvoker) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(AudioContextHostObject, outputLatency));
  addGetters(JSI_EXPORT_PROPERTY_GETTER(AudioContextHostObject, baseLatency));
  addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioContextHostObject, onerror));
  addFunctions(
      JSI_EXPORT_FUNCTION(AudioContextHostObject, close),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, suspend),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, createMediaElementSource));
}

AudioContextHostObject::~AudioContextHostObject() {
  std::static_pointer_cast<AudioContext>(context_)->assignOnErrorCallbackId(0);
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, close) {
  return createLifecyclePromise(
      ContextState::CLOSED, [](BaseAudioContext &context, const LifecycleResolver &resolver) {
        dynamic_cast<AudioContext &>(context).close(resolver);
      });
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, resume) {
  return createLifecyclePromise(
      ContextState::RUNNING, [](BaseAudioContext &context, const LifecycleResolver &resolver) {
        dynamic_cast<AudioContext &>(context).resume(resolver);
      });
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, suspend) {
  return createLifecyclePromise(
      ContextState::SUSPENDED, [](BaseAudioContext &context, const LifecycleResolver &resolver) {
        dynamic_cast<AudioContext &>(context).suspend(resolver);
      });
}

JSI_PROPERTY_GETTER_IMPL(AudioContextHostObject, outputLatency) {
  auto audioContext = std::static_pointer_cast<AudioContext>(context_);
  return {audioContext->getOutputLatency()};
}

JSI_PROPERTY_GETTER_IMPL(AudioContextHostObject, baseLatency) {
  auto audioContext = std::static_pointer_cast<AudioContext>(context_);
  return {audioContext->getBaseLatency()};
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

JSI_PROPERTY_SETTER_IMPL(AudioContextHostObject, onerror) {
  auto audioContext = std::static_pointer_cast<AudioContext>(context_);
  audioContext->assignOnErrorCallbackId(std::stoull(value.getString(runtime).utf8(runtime)));
}

} // namespace audioapi
