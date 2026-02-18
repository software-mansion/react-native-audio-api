#pragma once

#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <jsi/jsi.h>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace audioapi {
using namespace facebook;

/*
* # How to extract worklet from JavaScript argument
*
* To extract a shareable worklet from a JavaScript argument, use the following code:
*
* ```cpp
* auto worklet = worklets::extractSerializableWorkletFromArg(runtime, args[0]);
* ```
*
* This will return a shared pointer to the extracted worklet, or throw an error if the argument is invalid.
*/

class WorkletsRunner {
 public:
  explicit WorkletsRunner(
      std::weak_ptr<worklets::WorkletRuntime> weakRuntime,
      const std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
      bool shouldLockRuntime = true);
  WorkletsRunner(WorkletsRunner &&);
  ~WorkletsRunner();

  /// @brief Call the worklet function with the given arguments.
  /// @tparam ...Args
  /// @param ...args
  /// @return The result of the worklet function call.
  /// @note This method is unsafe and should be used with caution. It assumes that the runtime and worklet are valid and runtime is locked.
  template <typename... Args>
  inline jsi::Value callUnsafe(Args &&...args) {
    return getUnsafeWorklet().call(*unsafeRuntimePtr, std::forward<Args>(args)...);
  }

  /// @brief Call the worklet function with the given arguments.
  /// @tparam ...Args
  /// @param ...args
  /// @return The result of the worklet function call.
  /// @note This method is safe and will check if the runtime is available before calling the worklet. If the runtime is not available, it will return nullopt.
  template <typename... Args>
  inline std::optional<jsi::Value> call(Args &&...args) {
    return executeOnRuntimeGuarded([this, args...](jsi::Runtime &rt) -> jsi::Value {
      return callUnsafe(std::forward<Args>(args)...);
    });
  }

  /// @brief Execute a job on the UI runtime safely.
  /// @param job
  /// @return nullopt if the runtime is not available or the result of the job execution
  /// @note Execution is synchronous and will be guarded if shouldLockRuntime is true.
  inline std::optional<jsi::Value> executeOnRuntimeSync(
      const std::function<jsi::Value(jsi::Runtime &)> &&job) const noexcept(noexcept(job)) {
    if (shouldLockRuntime)
      return executeOnRuntimeGuarded(std::move(job));
    else
      return executeOnRuntimeUnsafe(std::move(job));
  }

 private:
  std::weak_ptr<worklets::WorkletRuntime> weakRuntime_;
  jsi::Runtime *unsafeRuntimePtr = nullptr;

  /// @note We want to avoid automatic destruction as
  /// when runtime is destroyed, underlying pointer will be invalid
  char unsafeWorklet[sizeof(jsi::Function)];
  bool workletInitialized = false;
  bool shouldLockRuntime = true;

  inline jsi::Function &getUnsafeWorklet() {
    return *reinterpret_cast<jsi::Function *>(&unsafeWorklet);
  }

  std::optional<jsi::Value> executeOnRuntimeGuarded(
      const std::function<jsi::Value(jsi::Runtime &)> &&job) const noexcept(noexcept(job));

  std::optional<jsi::Value> executeOnRuntimeUnsafe(
      const std::function<jsi::Value(jsi::Runtime &)> &&job) const noexcept(noexcept(job));
};

} // namespace audioapi
