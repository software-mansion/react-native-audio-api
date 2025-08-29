#include <audioapi/core/utils/worklets/UiWorkletsRunner.h>

namespace audioapi {

UiWorkletsRunner::UiWorkletsRunner(
    std::weak_ptr<worklets::WorkletRuntime> uiRuntime) noexcept
    : uiRuntime_(std::move(uiRuntime)) {}

jsi::Runtime *UiWorkletsRunner::getJSIRuntime() const noexcept {
  auto lockedRuntime = uiRuntime_.lock();
  if (lockedRuntime == nullptr) {
    return nullptr;
  }
#if RN_AUDIO_API_ENABLE_WORKLETS
  return &lockedRuntime->getJSIRuntime();
#else
  return nullptr;
#endif
}

}; // namespace audioapi
