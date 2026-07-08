#include <audioworklets/UIWorkletsRunner.h>

#include <atomic>
#include <memory>
#include <utility>

namespace audioworklets {

UIWorkletsRunner::UIWorkletsRunner(
    std::shared_ptr<worklets::WorkletRuntime> uiRuntime,
    std::shared_ptr<worklets::UIScheduler> uiScheduler,
    std::shared_ptr<worklets::Serializable> serializableWorklet)
    : uiRuntime_(std::move(uiRuntime)),
      uiScheduler_(std::move(uiScheduler)),
      serializableWorklet_(std::move(serializableWorklet)) {}

void UIWorkletsRunner::invokeOnUI(
    std::shared_ptr<std::vector<std::shared_ptr<audioapi::AudioArrayBuffer>>> channels,
    size_t channelCount,
    std::function<void()> onComplete) const {
  worklets::scheduleOnUI(
      uiScheduler_,
      [uiRuntime = uiRuntime_,
       serializableWorklet = serializableWorklet_,
       channels = std::move(channels),
       channelCount,
       onComplete = std::move(onComplete)]() {
        std::atomic_thread_fence(std::memory_order_acquire);

        jsi::Runtime &rt = worklets::getJSIRuntimeFromWorkletRuntime(uiRuntime);

        auto audioData = jsi::Array(rt, channelCount);
        for (size_t ch = 0; ch < channelCount; ++ch) {
          audioData.setValueAtIndex(rt, ch, jsi::ArrayBuffer(rt, (*channels)[ch]));
        }

        worklets::runSyncOnRuntime(
            uiRuntime,
            serializableWorklet,
            jsi::Value(std::move(audioData)),
            jsi::Value(static_cast<int>(channelCount)));

        if (onComplete) {
          onComplete();
        }
      });
}

} // namespace audioworklets
