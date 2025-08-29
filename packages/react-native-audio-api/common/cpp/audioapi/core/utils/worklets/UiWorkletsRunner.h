#pragma once

#include <jsi/jsi.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>

#include <functional>
#include <atomic>
#include <memory>
#include <utility>


namespace audioapi {
using namespace facebook;

class UiWorkletsRunner {
 public:
    explicit UiWorkletsRunner(std::weak_ptr<worklets::WorkletRuntime> weakUiRuntime) noexcept;

    inline jsi::Runtime* getJSIRuntime() const noexcept {
      auto strongRuntime = weakUiRuntime_.lock();
      if (strongRuntime == nullptr) {
         return nullptr;
      }
      #if RN_AUDIO_API_ENABLE_WORKLETS
      return &strongRuntime->getJSIRuntime();
      #else
      return nullptr;
      #endif
    }

    template<typename... Args>
    bool executeWorkletAsync(const std::shared_ptr<worklets::ShareableWorklet>& shareableWorklet, Args&&... args) {
      auto strongRuntime = weakUiRuntime_.lock();
      if (strongRuntime == nullptr) {
         return false;
      }

      #if RN_AUDIO_API_ENABLE_WORKLETS

      /// TODO change to use spsc channel and managed thread
      /// For now and test purposes it does the same thing as executeWorkletSync
      strongRuntime->runGuarded(shareableWorklet, std::forward<Args>(args)...);
      return true;

      #else
      return false;
      #endif
    }

    template<typename... Args>
    bool executeWorkletSync(const std::shared_ptr<worklets::ShareableWorklet>& shareableWorklet, Args&&... args) {
      auto strongRuntime = weakUiRuntime_.lock();
      if (strongRuntime == nullptr) {
         return false;
      }

      #if RN_AUDIO_API_ENABLE_WORKLETS

      strongRuntime->runGuarded(shareableWorklet, std::forward<Args>(args)...);
      return true;

      #else
      return false;
      #endif
    }

 private:
    std::weak_ptr<worklets::WorkletRuntime> weakUiRuntime_;
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
