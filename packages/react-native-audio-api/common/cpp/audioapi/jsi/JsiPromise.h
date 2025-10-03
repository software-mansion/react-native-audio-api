#pragma once

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <variant>
#include <thread>
#include <memory>
#include <string>
#include <utility>
#include <functional>

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

class PromiseVendor {
 public:
  PromiseVendor(jsi::Runtime *runtime, const std::shared_ptr<react::CallInvoker> &callInvoker): runtime_(runtime), callInvoker_(callInvoker) {}

  jsi::Value createPromise(const std::function<void(std::shared_ptr<Promise>)> &function);

  /// @brief Creates an asynchronous promise.
  /// @param function The function to execute asynchronously. It should return either a jsi::Value on success or a std::string error message on failure.
  /// @return The created promise.
  /// @note The function is executed on a different thread, and the promise is resolved or rejected based on the function's outcome.
  /// @example
  /// ```cpp
  /// auto promise = promiseVendor_->createAsyncPromise(
  ///     [](jsi::Runtime& rt) -> std::variant<jsi::Value, std::string> {
  ///    // Simulate some heavy work
  ///    std::this_thread::sleep_for(std::chrono::seconds(2));
  ///    return jsi::String::createFromUtf8(rt, "Promise resolved successfully!");
  ///  }
  /// );
  ///
  /// return promise;
  jsi::Value createAsyncPromise(std::function<std::variant<jsi::Value, std::string>(jsi::Runtime&)> &&function);

 private:
  jsi::Runtime *runtime_;
  std::shared_ptr<react::CallInvoker> callInvoker_;
};

} // namespace audioapi
