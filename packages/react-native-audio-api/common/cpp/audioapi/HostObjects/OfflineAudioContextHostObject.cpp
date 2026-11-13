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
  return createLifecyclePromise(
      ContextState::RUNNING, [](BaseAudioContext &context, LifecycleResolver resolver) {
        dynamic_cast<OfflineAudioContext &>(context).resume(std::move(resolver));
      });
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, suspend) {
  double when = args[0].getNumber();

  return createLifecyclePromise(
      ContextState::SUSPENDED, [when](BaseAudioContext &context, LifecycleResolver resolver) {
        dynamic_cast<OfflineAudioContext &>(context).suspend(when, std::move(resolver));
      });
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, startRendering) {
  context_->setPublishedState(ContextState::RUNNING);

  return createLifecyclePromise<ContextRenderResolver>(
      ContextState::CLOSED, [](BaseAudioContext &context, RenderResolver resolver) {
        dynamic_cast<OfflineAudioContext &>(context).startRendering(std::move(resolver));
      });
}

} // namespace audioapi
