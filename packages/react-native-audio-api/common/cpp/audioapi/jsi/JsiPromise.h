#pragma once

#include <ReactCommon/CallInvoker.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/ThreadPool.hpp>
#include <jsi/jsi.h>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <variant>

namespace audioapi {

using namespace facebook;

class Promise {
  struct Inner {
    std::shared_ptr<react::CallInvoker> callInvoker;
    jsi::Function resolve;
    jsi::Function reject;
  };

 public:
  explicit Promise(
      std::shared_ptr<react::CallInvoker> &&callInvoker,
      jsi::Function &&resolve,
      jsi::Function &&reject)
      : inner_(
            std::make_shared<Inner>(
                Inner{std::move(callInvoker), std::move(resolve), std::move(reject)})) {}

  Promise(const Promise &other) {
    inner_ = other.inner_;
  }
  Promise(Promise &&other) noexcept : inner_(std::move(other.inner_)) {}
  Promise &operator=(Promise &&other) noexcept {
    if (this != &other) {
      inner_ = std::move(other.inner_);
    }
    return *this;
  }

  void resolve(const std::function<jsi::Value(jsi::Runtime &)> &&resolver) const {
    auto inner = inner_;
    inner->callInvoker->invokeAsync(
        [inner = std::move(inner),
         resolver = std::forward<decltype(resolver)>(resolver)](jsi::Runtime &runtime) -> void {
          auto valueShared = std::make_shared<jsi::Value>(resolver(runtime));
          inner->resolve.call(runtime, *valueShared);
        });
  }

  void reject(const std::string &errorMessage) const {
    auto inner = inner_;
    inner->callInvoker->invokeAsync(
        [inner = std::move(inner), errorMessage](jsi::Runtime &runtime) -> void {
          auto error = jsi::JSError(runtime, errorMessage);
          inner->reject.call(runtime, error.value());
        });
  }

 private:
  std::shared_ptr<Inner> inner_;
};

using PromiseResolver = std::function<std::variant<jsi::Value, std::string>(jsi::Runtime &)>;

class PromiseVendor {
 public:
  PromiseVendor(jsi::Runtime *runtime, const std::shared_ptr<react::CallInvoker> &callInvoker)
      : runtime_(runtime),
        callInvoker_(callInvoker),
        threadPool_(
            std::make_shared<ThreadPool>(
                audioapi::PROMISE_VENDOR_THREAD_POOL_WORKER_COUNT,
                audioapi::PROMISE_VENDOR_THREAD_POOL_LOAD_BALANCER_QUEUE_SIZE,
                audioapi::PROMISE_VENDOR_THREAD_POOL_WORKER_QUEUE_SIZE)) {}

  /// @brief Creates an asynchronous promise.
  /// @param function The function to execute asynchronously. It should return either a jsi::Value on success or a std::string error message on failure.
  /// @return The created promise.
  /// @note The function is executed on a different thread, and the promise is resolved or rejected based on the function's outcome.
  /// @note IMPORTANT: This function is not thread-safe and should be called from a single thread only. (comes from underlying ThreadPool implementation)
  /// @example
  /// ```cpp
  /// auto promise = promiseVendor_->createAsyncPromise(
  ///     []() -> PromiseResolver {
  ///    // Simulate some heavy work on a background thread
  ///    std::this_thread::sleep_for(std::chrono::seconds(2));
  ///    return [](jsi::Runtime &rt) -> std::variant<jsi::Value, std::string> {
  ///      // Prepare and return the result on javascript thread
  ///      return jsi::String::createFromUtf8(rt, "Promise resolved successfully!");
  ///    };
  ///  }
  /// );
  ///
  /// return promise;
  jsi::Value createAsyncPromise(std::function<PromiseResolver()> &&function);

  /// @brief Creates an asynchronous promise.
  /// @param function The function to execute asynchronously. It receives a Promise object to resolve or reject the promise.
  /// @return The created promise.
  /// @note The function is executed on a different thread, the promise should be resolved or rejected using the provided Promise object.
  /// @note IMPORTANT: This function is not thread-safe and should be called from a single thread only. (comes from underlying ThreadPool implementation)
  jsi::Value createAsyncPromise(std::function<void(Promise &&)> &&function);

 private:
  jsi::Runtime *runtime_;
  std::shared_ptr<react::CallInvoker> callInvoker_;
  std::shared_ptr<ThreadPool> threadPool_;

  static void asyncPromiseJob(
      std::shared_ptr<react::CallInvoker> callInvoker,
      std::function<PromiseResolver()> &&function,
      std::shared_ptr<jsi::Function> &&resolve,
      std::shared_ptr<jsi::Function> &&reject);
};

} // namespace audioapi
