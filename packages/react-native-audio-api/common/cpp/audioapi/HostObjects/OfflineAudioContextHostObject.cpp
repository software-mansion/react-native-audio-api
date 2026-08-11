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
          callInvoker,
          numberOfChannels) {
  addFunctions(
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, suspend),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, startRendering));
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, resume) {
  return promiseVendor_->createPromise([this](Promise &&promise) {
    auto contextPromise = ContextPromiseResolver<void>::makeContextPromiseResolver(
        std::move(promise), context_, ContextState::RUNNING);
    context_->scheduleContextPromise([contextPromise](BaseAudioContext &context) {
      dynamic_cast<OfflineAudioContext &>(context).resume(contextPromise);
    });
  });
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, suspend) {
  double when = args[0].getNumber();
  return promiseVendor_->createPromise([this, when](Promise &&promise) {
    auto contextPromise = ContextPromiseResolver<void>::makeContextPromiseResolver(
        std::move(promise), context_, ContextState::SUSPENDED);
    context_->scheduleContextPromise([contextPromise, when](BaseAudioContext &context) {
      dynamic_cast<OfflineAudioContext &>(context).suspend(when, contextPromise);
    });
  });
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, startRendering) {
  return promiseVendor_->createPromise([this](Promise &&promise) {
    auto resultPromise = OfflineAudioContextResultPromise::makeOfflineAudioContextResultResolver(
        std::move(promise), context_);
    context_->scheduleContextPromise([resultPromise](BaseAudioContext &context) {
      dynamic_cast<OfflineAudioContext &>(context).startRendering(resultPromise);
    });
  });
}

} // namespace audioapi
