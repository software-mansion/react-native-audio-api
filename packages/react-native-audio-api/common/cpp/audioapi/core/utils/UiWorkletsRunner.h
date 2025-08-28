#pragma once

#include <jsi/jsi.h>

#include <functional>
#include <atomic>
#include <memory>

#define RN_AUDIO_API_ENABLE_WORKLETS 1

#if RN_AUDIO_API_ENABLE_WORKLETS
#include <worklets/WorkletRuntime/WorkletRuntime.h>
#include <worklets/SharedItems/Shareables.h>
#else
/// @brief Dummy implementation of worklets for non-worklet builds they should do nothing and mock necessary methods
/// @note It helps to reduce compile time branching across codebase
/// @note If you need to base some c++ implementation on if the worklets are enabled use `#if RN_AUDIO_API_ENABLE_WORKLETS`
namespace worklets {
struct WorkletRuntime {};
struct ShareableWorklet {};
} // namespace worklets
#endif

namespace audioapi {
using namespace facebook;

class UiWorkletsRunner {
 public:
    explicit UiWorkletsRunner(std::weak_ptr<worklets::WorkletRuntime> uiRuntime) noexcept;

    jsi::Runtime* getJSIRuntime() const noexcept;

    template<typename... Args>
    bool executeWorkletAsync(const std::shared_ptr<worklets::ShareableWorklet>& shareableWorklet, Args&&... args);

    template<typename... Args>
    bool executeWorkletSync(const std::shared_ptr<worklets::ShareableWorklet>& shareableWorklet, Args&&... args);

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

// Template methods definitions
#include <audioapi/core/utils/UiWorkletsRunner.tpp>
