#pragma once

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/types/ContextState.h>
#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <jsi/jsi.h>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

/// Settles one context lifecycle promise (resume / suspend / close / startRendering).
/// A resolve moves the context to the state the resolver was made for, publishes that state on
/// the JS thread in the task that resolves the JS promise, then fires `statechange`. A reject
/// only rejects the JS promise. Subclasses fix what the promise resolves with.
class ContextPromiseResolver {
 public:
  void reject(const std::string &message) const {
    jsiPromise_.reject(message);
  }

 protected:
  ContextPromiseResolver(
      Promise &&promise,
      std::shared_ptr<BaseAudioContext> context,
      ContextState nextState)
      : jsiPromise_(std::move(promise)), context_(std::move(context)), nextState_(nextState) {}

  void settle(std::function<jsi::Value(jsi::Runtime &)> &&toValue) const {
    // Spec: update the state attribute in the same follow-up task that resolves the promise
    // (before statechange reactions).
    context_->setState(nextState_);
    jsiPromise_.resolve([context = context_, nextState = nextState_, toValue = std::move(toValue)](
                            jsi::Runtime &runtime) {
      context->setPublishedState(nextState);
      return toValue(runtime);
    });
    // Behind the resolve enqueue; the TS layer orders the offline `complete` event after this
    // statechange lands.
    context_->dispatchStateChange(nextState_);
  }

 private:
  Promise jsiPromise_;
  std::shared_ptr<BaseAudioContext> context_;
  ContextState nextState_;
};

/// Resolves with `undefined`: resume / suspend / close.
class ContextStateResolver final : public ContextPromiseResolver {
 public:
  ContextStateResolver(
      Promise &&promise,
      std::shared_ptr<BaseAudioContext> context,
      ContextState nextState)
      : ContextPromiseResolver(std::move(promise), std::move(context), nextState) {}

  void resolve() const {
    settle([](jsi::Runtime &) { return jsi::Value::undefined(); });
  }
};

/// Resolves with the rendered `AudioBuffer`: offline `startRendering`.
class ContextRenderResolver final : public ContextPromiseResolver {
 public:
  ContextRenderResolver(
      Promise &&promise,
      std::shared_ptr<BaseAudioContext> context,
      ContextState nextState)
      : ContextPromiseResolver(std::move(promise), std::move(context), nextState) {}

  void resolve(const std::shared_ptr<AudioBuffer> &buffer) const {
    settle([buffer](jsi::Runtime &runtime) {
      return AudioBufferHostObject::createJsObject(runtime, buffer);
    });
  }
};

} // namespace audioapi
