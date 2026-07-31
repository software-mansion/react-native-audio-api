#include <audioapi/jsi/ContextPromiseResolver.hpp>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/types/ContextState.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

template <typename T>
std::shared_ptr<ContextPromiseResolver<void>> ContextPromiseResolver<T>::makeContextPromiseResolver(
    Promise &&promise,
    const std::shared_ptr<BaseAudioContext> &audioContext,
    ContextState nextState)
  requires std::same_as<T, void>
{
  auto jsiPromise = std::make_shared<Promise>(std::move(promise));
  return std::make_shared<ContextPromiseResolver<void>>(
      [jsiPromise, audioContext, nextState]() {
        // Spec: update the state attribute in the same follow-up task that
        // resolves the lifecycle promise (before statechange reactions).
        audioContext->setState(nextState);
        jsiPromise->resolve([](jsi::Runtime &runtime) { return jsi::Value::undefined(); });
      },
      [jsiPromise](const std::string &message) { jsiPromise->reject(message); });
}

template <typename T>
std::shared_ptr<OfflineAudioContextResultPromise>
ContextPromiseResolver<T>::makeOfflineAudioContextResultResolver(
    Promise &&promise,
    const std::shared_ptr<BaseAudioContext> &audioContext)
  requires(!std::same_as<T, void>)
{
  auto jsiPromise = std::make_shared<Promise>(std::move(promise));
  return std::make_shared<OfflineAudioContextResultPromise>(
      [jsiPromise, audioContext](const std::shared_ptr<AudioBuffer> &audioBuffer) {
        // Spec: update the state attribute in the same follow-up task that
        // resolves the startRendering promise (before statechange reactions).
        audioContext->setState(ContextState::CLOSED);
        auto audioBufferHostObject = std::make_shared<AudioBufferHostObject>(audioBuffer);
        jsiPromise->resolve([audioBufferHostObject](jsi::Runtime &runtime) {
          return jsi::Object::createFromHostObject(runtime, audioBufferHostObject);
        });
      },
      [jsiPromise](const std::string &message) { jsiPromise->reject(message); });
}

template std::shared_ptr<ContextPromiseResolver<void>>
ContextPromiseResolver<void>::makeContextPromiseResolver(
    Promise &&,
    const std::shared_ptr<BaseAudioContext> &,
    ContextState);

template std::shared_ptr<OfflineAudioContextResultPromise>
ContextPromiseResolver<std::shared_ptr<AudioBuffer>>::makeOfflineAudioContextResultResolver(
    Promise &&,
    const std::shared_ptr<BaseAudioContext> &);

} // namespace audioapi
