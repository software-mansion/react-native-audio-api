#include <audioapi/HostObjects/AudioContextHostObject.h>

#include <audioapi/HostObjects/sources/AudioFileSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/MediaElementAudioSourceNodeHostObject.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/types/ContextState.h>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {
namespace {

std::shared_ptr<ContextPromise> makeContextPromise(
    Promise &&promise,
    const std::shared_ptr<AudioContext> &audioContext,
    ContextState nextState) {
  auto jsiPromise = std::make_shared<Promise>(std::move(promise));
  return std::make_shared<ContextPromise>(
      [jsiPromise, audioContext, nextState]() {
        // Spec: update the state attribute in the same follow-up task that
        // resolves the lifecycle promise (before statechange reactions).
        audioContext->setState(nextState);
        jsiPromise->resolve([](jsi::Runtime &runtime) { return jsi::Value::undefined(); });
      },
      [jsiPromise](const std::string &message) { jsiPromise->reject(message); });
}

} // namespace

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
  context_->getGraph()->collectDisposedNodes();
  auto audioContext = std::static_pointer_cast<AudioContext>(context_);
  return promiseVendor_->createPromise([audioContext = std::move(audioContext)](Promise &&promise) {
    auto contextPromise =
        makeContextPromise(std::move(promise), audioContext, ContextState::CLOSED);
    audioContext->scheduleAudioEvent([contextPromise](BaseAudioContext &context) {
      dynamic_cast<AudioContext &>(context).close(contextPromise);
    });
  });
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, resume) {
  context_->getGraph()->collectDisposedNodes();
  auto audioContext = std::static_pointer_cast<AudioContext>(context_);
  return promiseVendor_->createPromise([audioContext = std::move(audioContext)](Promise &&promise) {
    auto contextPromise =
        makeContextPromise(std::move(promise), audioContext, ContextState::RUNNING);
    audioContext->scheduleAudioEvent([contextPromise](BaseAudioContext &context) {
      dynamic_cast<AudioContext &>(context).resume(contextPromise);
    });
  });
}

JSI_HOST_FUNCTION_IMPL(AudioContextHostObject, suspend) {
  context_->getGraph()->collectDisposedNodes();
  auto audioContext = std::static_pointer_cast<AudioContext>(context_);
  return promiseVendor_->createPromise([audioContext = std::move(audioContext)](Promise &&promise) {
    auto contextPromise =
        makeContextPromise(std::move(promise), audioContext, ContextState::SUSPENDED);
    audioContext->scheduleAudioEvent([contextPromise](BaseAudioContext &context) {
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
