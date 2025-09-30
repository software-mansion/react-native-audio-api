#pragma once

#include <jsi/jsi.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>

#include <functional>
#include <atomic>
#include <memory>
#include <utility>
#include <optional>

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
    explicit WorkletsRunner(std::weak_ptr<worklets::WorkletRuntime> weakUiRuntime) noexcept;

    /// @brief Execute a job on the UI runtime safely.
    /// @param job
    /// @return nullopt if the runtime is not available or the result of the job execution
    /// @note Execution is synchronous
    std::optional<jsi::Value> executeOnRuntimeGuardedSync(const std::function<jsi::Value(jsi::Runtime&)>&& job) const noexcept(noexcept(job)) {
      auto strongRuntime = weakUiRuntime_.lock();
      if (strongRuntime == nullptr) {
         return std::nullopt;
      }
      #if RN_AUDIO_API_ENABLE_WORKLETS
      return strongRuntime->executeSync(std::move(job));
      #else
      return std::nullopt;
      #endif
    }

    /// @brief Execute a worklet with the given arguments.
    /// @tparam ...Args
    /// @param shareableWorklet
    /// @param ...args
    /// @note Execution is synchronous, this method can be used in `executeOnRuntimeGuardedSync` and `...Async` methods arguments
    /// @return nullopt if the runtime is not available or the result of the worklet execution
    template<typename... Args>
    std::optional<jsi::Value> executeWorklet(const std::shared_ptr<worklets::SerializableWorklet>& shareableWorklet, Args&&... args) {
      auto strongRuntime = weakUiRuntime_.lock();
      if (strongRuntime == nullptr) {
         return std::nullopt;
      }

      #if RN_AUDIO_API_ENABLE_WORKLETS

      return strongRuntime->runGuarded(shareableWorklet, std::forward<Args>(args)...);

      #else
      return std::nullopt;
      #endif
    }

 private:
    std::weak_ptr<worklets::WorkletRuntime> weakUiRuntime_;
};

} // namespace audioapi
