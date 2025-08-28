#include <audioapi/core/utils/UiWorkletsRunner.h>

namespace audioapi {

template<typename... Args>
bool UiWorkletsRunner::executeWorkletAsync(const std::shared_ptr<worklets::ShareableWorklet>& shareableWorklet, Args&&... args) {
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
bool UiWorkletsRunner::executeWorkletSync(const std::shared_ptr<worklets::ShareableWorklet>& shareableWorklet, Args&&... args) {
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

} // namespace audioapi
