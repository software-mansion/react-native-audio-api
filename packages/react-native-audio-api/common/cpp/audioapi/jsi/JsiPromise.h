#pragma once

#include <ReactCommon/CallInvoker.h>
#include <audioapi/utils/ITaskExecutor.h>
#include <audioapi/utils/PooledTaskExecutor.hpp>
#include <jsi/jsi.h>
#include <functional>
#include <memory>
#include <string>
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
            std::make_shared<Inner>(Inner{
                .callInvoker = std::move(callInvoker),
                .resolve = std::move(resolve),
                .reject = std::move(reject)})) {}

  Promise(const Promise &other) = default;
  Promise(Promise &&other) noexcept = default;
  Promise &operator=(Promise &&other) noexcept = default;

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
  /// @brief Vendor whose `createAsyncPromise` bodies run on a multi-worker pool.
  PromiseVendor(jsi::Runtime *runtime, const std::shared_ptr<react::CallInvoker> &callInvoker)
      : PromiseVendor(runtime, callInvoker, std::make_shared<PooledTaskExecutor>()) {}

  /// @param executor Runs `createAsyncPromise` bodies. A `SerialTaskExecutor` makes the vendor
  /// a lane: bodies run in the order the promises were created.
  PromiseVendor(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker,
      std::shared_ptr<ITaskExecutor> executor)
      : runtime_(runtime), callInvoker_(callInvoker), executor_(std::move(executor)) {}

  /// @brief Runs @p task on the vendor's executor with no JS promise attached.
  /// On a serial executor it queues behind every promise body created before it, so it can be
  /// used from places that have no runtime, such as a HostObject destructor.
  /// @note Same single-producer rule as `createAsyncPromise`.
  void scheduleDetached(std::function<void()> &&task);

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

  /// @brief Creates a promise and runs `function` synchronously on the calling thread.
  /// @param function Receives a Promise to resolve/reject later (e.g. after an
  /// audio-thread control message). Unlike `createAsyncPromise`, this does not
  /// hop to the promise-vendor thread pool — use it when the caller must remain
  /// the SPSC producer (JS thread) for `scheduleAudioEvent`.
  jsi::Value createPromise(std::function<void(Promise &&)> &&function);

 private:
  jsi::Runtime *runtime_;
  std::shared_ptr<react::CallInvoker> callInvoker_;
  std::shared_ptr<ITaskExecutor> executor_;

  static void asyncPromiseJob(
      const std::shared_ptr<react::CallInvoker> &callInvoker,
      const std::function<PromiseResolver()> &function,
      std::shared_ptr<jsi::Function> &&resolve,
      std::shared_ptr<jsi::Function> &&reject);
};

} // namespace audioapi
