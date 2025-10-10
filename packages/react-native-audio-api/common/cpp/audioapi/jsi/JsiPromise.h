#pragma once


#include <audioapi/core/utils/Constants.h>
#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <variant>
#include <thread>
#include <memory>
#include <string>
#include <utility>
#include <functional>
#include <audioapi/utils/ThreadPool.hpp>

namespace audioapi {

using namespace facebook;

class Promise {
 public:
  Promise(std::function<void(const std::function<jsi::Value(jsi::Runtime&)>)> resolve, std::function<void(const std::string &)> reject) : resolve_(std::move(resolve)), reject_(std::move(reject)) {}

  void resolve(const std::function<jsi::Value(jsi::Runtime&)> &resolver) {
    resolve_(std::forward<const std::function<jsi::Value(jsi::Runtime&)>>(resolver));
  }

  void reject(const std::string &errorMessage) {
    reject_(errorMessage);
  }

 private:
  std::function<void(const std::function<jsi::Value(jsi::Runtime&)>)> resolve_;
  std::function<void(const std::string &)> reject_;
};

using PromiseResolver = std::function<std::variant<jsi::Value, std::string>(jsi::Runtime&)>;

class PromiseVendor {
 public:
  PromiseVendor(jsi::Runtime *runtime, const std::shared_ptr<react::CallInvoker> &callInvoker):
    runtime_(runtime), callInvoker_(callInvoker), threadPool_(std::make_shared<ThreadPool>(
      audioapi::PROMISE_VENDOR_THREAD_POOL_WORKER_COUNT,
      audioapi::PROMISE_VENDOR_THREAD_POOL_LOAD_BALANCER_QUEUE_SIZE,
      audioapi::PROMISE_VENDOR_THREAD_POOL_WORKER_QUEUE_SIZE)) {}

  jsi::Value createPromise(const std::function<void(std::shared_ptr<Promise>)> &function);

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

 private:
  jsi::Runtime *runtime_;
  std::shared_ptr<react::CallInvoker> callInvoker_;
  std::shared_ptr<ThreadPool> threadPool_;

  static void asyncPromiseJob(
    std::shared_ptr<react::CallInvoker> callInvoker,
    std::function<PromiseResolver()> &&function,
    std::shared_ptr<jsi::Function> &&resolve,
    std::shared_ptr<jsi::Function> &&reject
    );
};

} // namespace audioapi
