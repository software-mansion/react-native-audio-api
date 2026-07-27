#include <audioapi/HostObjects/OfflineAudioContextHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {
namespace {

std::shared_ptr<ContextPromise> makeContextPromise(Promise &&promise) {
  auto jsiPromise = std::make_shared<Promise>(std::move(promise));
  return std::make_shared<ContextPromise>(
      [jsiPromise]() {
        jsiPromise->resolve([](jsi::Runtime &runtime) { return jsi::Value::undefined(); });
      },
      [jsiPromise](const std::string &message) { jsiPromise->reject(message); });
}

std::shared_ptr<OfflineAudioContextResultPromise> makeResultPromise(Promise &&promise) {
  auto jsiPromise = std::make_shared<Promise>(std::move(promise));
  return std::make_shared<OfflineAudioContextResultPromise>(
      [jsiPromise](const std::shared_ptr<AudioBuffer> &audioBuffer) {
        auto audioBufferHostObject = std::make_shared<AudioBufferHostObject>(audioBuffer);
        jsiPromise->resolve([audioBufferHostObject](jsi::Runtime &runtime) {
          return jsi::Object::createFromHostObject(runtime, audioBufferHostObject);
        });
      },
      [jsiPromise](const std::string &message) { jsiPromise->reject(message); });
}

} // namespace

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
  return promiseVendor_->createPromise([audioContext](Promise &&promise) {
    auto contextPromise = makeContextPromise(std::move(promise));
    audioContext->scheduleAudioEvent([contextPromise](BaseAudioContext &context) {
      dynamic_cast<OfflineAudioContext &>(context).resume(contextPromise);
    });
  });
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, suspend) {
  context_->getGraph()->collectDisposedNodes();
  double when = args[0].getNumber();
  auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);

  return promiseVendor_->createPromise([audioContext, when](Promise &&promise) {
    auto contextPromise = makeContextPromise(std::move(promise));
    audioContext->scheduleAudioEvent([contextPromise, when](BaseAudioContext &context) {
      dynamic_cast<OfflineAudioContext &>(context).suspend(when, contextPromise);
    });
  });
}

JSI_HOST_FUNCTION_IMPL(OfflineAudioContextHostObject, startRendering) {
  auto audioContext = std::static_pointer_cast<OfflineAudioContext>(context_);
  return promiseVendor_->createPromise([audioContext](Promise &&promise) {
    auto resultPromise = makeResultPromise(std::move(promise));
    audioContext->scheduleAudioEvent([resultPromise](BaseAudioContext &context) {
      dynamic_cast<OfflineAudioContext &>(context).startRendering(resultPromise);
    });
  });
}

} // namespace audioapi
