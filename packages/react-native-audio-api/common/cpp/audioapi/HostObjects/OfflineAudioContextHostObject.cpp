#include <audioapi/HostObjects/OfflineAudioContextHostObject.h>

#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/types/ContextState.h>
#include <memory>

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
      ContextState::RUNNING, [](BaseAudioContext &context, const LifecycleResolver &resolver) {
        dynamic_cast<OfflineAudioContext &>(context).resume(resolver);
      });
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, suspend) {
  double when = args[0].getNumber();

  return createLifecyclePromise(
      ContextState::SUSPENDED,
      [when](BaseAudioContext &context, const LifecycleResolver &resolver) {
        dynamic_cast<OfflineAudioContext &>(context).suspend(when, resolver);
      });
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, startRendering) {
  context_->setPublishedState(ContextState::RUNNING);

  return createLifecyclePromise<ContextRenderResolver>(
      ContextState::CLOSED, [](BaseAudioContext &context, const RenderResolver &resolver) {
        dynamic_cast<OfflineAudioContext &>(context).startRendering(resolver);
      });
}

} // namespace audioapi
