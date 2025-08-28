#pragma once

#include <jsi/jsi.h>
#include <audioapi/core/utils/UiWorkletsSafeIncludes.h>

#include <functional>
#include <atomic>
#include <memory>
#include <utility>


namespace audioapi {
using namespace facebook;

class UiWorkletsRunner {
 public:
    explicit UiWorkletsRunner(std::weak_ptr<worklets::WorkletRuntime> uiRuntime) noexcept;

    jsi::Runtime* getJSIRuntime() const noexcept;

    template<typename... Args>
    bool executeWorkletAsync(const std::shared_ptr<worklets::ShareableWorklet>& shareableWorklet, Args&&... args) {
      auto lockedRuntime = uiRuntime_.lock();
      if (lockedRuntime == nullptr) {
         return false;
      }

      #if RN_AUDIO_API_ENABLE_WORKLETS

      /// TODO change to use spsc channel and managed thread
      /// For now and test purposes it does the same thing as executeWorkletSync
      lockedRuntime->runGuarded(shareableWorklet, std::forward<Args>(args)...);
      return true;

      #else
      return false;
      #endif
    }

    template<typename... Args>
    bool executeWorkletSync(const std::shared_ptr<worklets::ShareableWorklet>& shareableWorklet, Args&&... args) {
      auto lockedRuntime = uiRuntime_.lock();
      if (lockedRuntime == nullptr) {
         return false;
      }

      #if RN_AUDIO_API_ENABLE_WORKLETS

      lockedRuntime->runGuarded(shareableWorklet, std::forward<Args>(args)...);
      return true;

      #else
      return false;
      #endif
    }

 private:
    std::weak_ptr<worklets::WorkletRuntime> uiRuntime_;
};

/*
* # How to extract worklet from JavaScript argument
*
* To extract a shareable worklet from a JavaScript argument, use the following code:
*
* ```cpp
* auto worklet = worklets::extractShareableWorkletFromArg(runtime, args[0]);
* ```
*
* This will return a shared pointer to the extracted worklet, or throw an error if the argument is invalid.
*/


} // namespace audioapi
