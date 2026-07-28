#include <audioapi/HostObjects/OfflineAudioContextHostObject.h>

#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/types/ContextState.h>
#include <memory>
#include <utility>

namespace audioapi {

OfflineAudioContextHostObject::OfflineAudioContextHostObject(
    int numberOfChannels,
    size_t length,
    float sampleRate,
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker)
    : BaseAudioContextHostObject(
          std::make_shared<OfflineAudioContext>(
              numberOfChannels,
              length,
              sampleRate,
              audioEventHandlerRegistry),
          runtime,
          callInvoker) {
  addFunctions(
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, suspend),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, startRendering));
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, resume) {
  context_->getGraph()->collectDisposedNodes();
  auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);
  return promiseVendor_->createPromise([this, audioContext](Promise &&promise) {
    auto contextPromise = ContextPromiseResolverVoid::makeContextPromise(
        std::move(promise), context_, ContextState::RUNNING);
    audioContext->scheduleAudioEvent([contextPromise](BaseAudioContext &context) {
      dynamic_cast<OfflineAudioContext &>(context).resume(contextPromise);
    });
  });
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, suspend) {
  context_->getGraph()->collectDisposedNodes();
  double when = args[0].getNumber();
  auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);

  return promiseVendor_->createPromise([this, audioContext, when](Promise &&promise) {
    auto contextPromise = ContextPromiseResolverVoid::makeContextPromise(
        std::move(promise), context_, ContextState::SUSPENDED);
    audioContext->scheduleAudioEvent([contextPromise, when](BaseAudioContext &context) {
      dynamic_cast<OfflineAudioContext &>(context).suspend(when, contextPromise);
    });
  });
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, startRendering) {
  auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);
  return promiseVendor_->createPromise([audioContext](Promise &&promise) {
    auto resultPromise = OfflineAudioContextResultPromise::makeOfflineAudioContextResultPromise(
        std::move(promise), audioContext);
    audioContext->scheduleAudioEvent([resultPromise](BaseAudioContext &context) {
      dynamic_cast<OfflineAudioContext &>(context).startRendering(resultPromise);
    });
  });
}

} // namespace audioapi
