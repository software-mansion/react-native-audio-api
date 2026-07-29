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
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker)
    : BaseAudioContextHostObject(
          std::make_shared<AudioContext>(sampleRate, audioEventHandlerRegistry),
          runtime,
          callInvoker) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(AudioContextHostObject, outputLatency));
  addGetters(JSI_EXPORT_PROPERTY_GETTER(AudioContextHostObject, baseLatency));
  addFunctions(
      JSI_EXPORT_FUNCTION(AudioContextHostObject, close),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, suspend),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, createMediaElementSource));
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, close) {
  return promiseVendor_->createPromise([this](Promise &&promise) {
    auto contextPromise = ContextPromiseResolverVoid::makeContextPromiseResolver(
        std::move(promise), context_, ContextState::CLOSED);
    context_->scheduleContextPromise([contextPromise](BaseAudioContext &context) {
      dynamic_cast<AudioContext &>(context).close(contextPromise);
    });
  });
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, resume) {
  return promiseVendor_->createPromise([this](Promise &&promise) {
    auto contextPromise = ContextPromiseResolverVoid::makeContextPromiseResolver(
        std::move(promise), context_, ContextState::RUNNING);
    context_->scheduleContextPromise([contextPromise](BaseAudioContext &context) {
      dynamic_cast<AudioContext &>(context).resume(contextPromise);
    });
  });
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, suspend) {
  return promiseVendor_->createPromise([this](Promise &&promise) {
    auto contextPromise = ContextPromiseResolverVoid::makeContextPromiseResolver(
        std::move(promise), context_, ContextState::SUSPENDED);
    context_->scheduleContextPromise([contextPromise](BaseAudioContext &context) {
      dynamic_cast<AudioContext &>(context).suspend(contextPromise);
    });
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

} // namespace audioapi
