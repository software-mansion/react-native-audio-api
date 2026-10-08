#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/types/ContextState.h>
#include <audioapi/jsi/ContextPromiseResolver.hpp>
#include <audioapi/jsi/HostObject.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <concepts>
#include <cstddef>
#include <memory>
#include <utility>

namespace audioapi {
using namespace facebook;

class AudioDestinationNodeHostObject;
class AudioListenerHostObject;

using LifecycleResolver = std::shared_ptr<ContextPromiseResolver<void>>;

/// A lifecycle body: receives the context and the resolver the core method settles.
template <typename F>
concept LifecycleOperation = std::invocable<F, BaseAudioContext &, const LifecycleResolver &>;

class BaseAudioContextHostObject : public HostObject {
 public:
  explicit BaseAudioContextHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker,
      int destinationChannelCount = 2);

  ~BaseAudioContextHostObject() override;

  JSI_PROPERTY_GETTER_DECL(destination);
  JSI_PROPERTY_GETTER_DECL(state);
  JSI_PROPERTY_SETTER_DECL(onstatechange);

  JSI_PROPERTY_GETTER_DECL(listener);
  JSI_PROPERTY_GETTER_DECL(sampleRate);
  JSI_PROPERTY_GETTER_DECL(currentTime);

  JSI_HOST_FUNCTION_DECL(createRecorderAdapter);
  JSI_HOST_FUNCTION_DECL(createOscillator);
  JSI_HOST_FUNCTION_DECL(createConstantSource);
  JSI_HOST_FUNCTION_DECL(createGain);
  JSI_HOST_FUNCTION_DECL(createStereoPanner);
  JSI_HOST_FUNCTION_DECL(createPanner);
  JSI_HOST_FUNCTION_DECL(createBiquadFilter);
  JSI_HOST_FUNCTION_DECL(createIIRFilter);
  JSI_HOST_FUNCTION_DECL(createBufferSource);
  JSI_HOST_FUNCTION_DECL(createFileSource);
  JSI_HOST_FUNCTION_DECL(createBufferQueueSource);
  JSI_HOST_FUNCTION_DECL(createPeriodicWave);
  JSI_HOST_FUNCTION_DECL(createAnalyser);
  JSI_HOST_FUNCTION_DECL(createConvolver);
  JSI_HOST_FUNCTION_DECL(createWaveShaper);
  JSI_HOST_FUNCTION_DECL(createDelay);
  JSI_HOST_FUNCTION_DECL(createChannelMerger);
  JSI_HOST_FUNCTION_DECL(createChannelSplitter);

  /// @brief Access the underlying C++ audio context.
  /// @return The underlying C++ audio context.
  [[nodiscard]] const std::shared_ptr<BaseAudioContext> &getContext() const {
    return context_;
  }

 protected:
  /// @brief Queues a lifecycle operation (resume / suspend / close) on the vendor's serial lane
  /// and returns its JS promise. A successful settle moves the context to @p nextState.
  /// Runs under the context's driver mutex, in the order JS issued the calls, and the captured
  /// context keeps itself alive until its queued operation has run.
  template <LifecycleOperation Operation>
  jsi::Value createLifecyclePromise(ContextState nextState, Operation &&operation) {
    return promiseVendor_->createAsyncPromise(
        [context = context_, nextState, operation = std::forward<Operation>(operation)](
            Promise &&promise) {
          auto resolver = ContextPromiseResolver<void>::makeContextPromiseResolver(
              std::move(promise), context, nextState);
          context->runLifecycleOperation([&] { operation(*context, resolver); });
        });
  }

  std::shared_ptr<BaseAudioContext> context_;
  std::shared_ptr<PromiseVendor> promiseVendor_;
  std::shared_ptr<react::CallInvoker> callInvoker_;

  std::shared_ptr<AudioDestinationNodeHostObject> destination_;
  std::shared_ptr<AudioListenerHostObject> listener_;
};
} // namespace audioapi
