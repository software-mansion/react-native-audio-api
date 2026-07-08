#include <audioworklets/UIWorkletsRunner.h>

#include <atomic>
#include <memory>
#include <utility>

namespace audioworklets {

UIWorkletsRunner::UIWorkletsRunner(
    std::shared_ptr<worklets::WorkletRuntime> uiRuntime,
    std::shared_ptr<worklets::UIScheduler> uiScheduler,
    std::shared_ptr<worklets::Serializable> serializableWorklet)
    : alive_(std::make_shared<std::atomic<bool>>(true)),
      uiRuntime_(uiRuntime),
      uiScheduler_(uiScheduler),
      serializableWorklet_(std::move(serializableWorklet)) {}

void UIWorkletsRunner::deactivate() {
  alive_->store(false, std::memory_order_release);
}

bool UIWorkletsRunner::isActive() const {
  return alive_->load(std::memory_order_acquire);
}

void UIWorkletsRunner::invokeOnUI(
    std::shared_ptr<std::vector<std::shared_ptr<audioapi::AudioArrayBuffer>>> channels,
    size_t channelCount,
    std::function<void()> onComplete) const {
  if (!isActive() || !serializableWorklet_) {
    if (onComplete) {
      onComplete();
    }
    return;
  }

  auto uiRuntime = uiRuntime_.lock();
  auto uiScheduler = uiScheduler_.lock();
  if (!uiRuntime || !uiScheduler) {
    if (onComplete) {
      onComplete();
    }
    return;
  }

  auto alive = alive_;

  worklets::scheduleOnUI(
      uiScheduler,
      [alive,
       weakRuntime = uiRuntime_,
       weakScheduler = uiScheduler_,
       serializableWorklet = serializableWorklet_,
       channels = std::move(channels),
       channelCount,
       onComplete = std::move(onComplete)]() {
        if (!alive->load(std::memory_order_acquire)) {
          if (onComplete) {
            onComplete();
          }
          return;
        }

        auto runtime = weakRuntime.lock();
        if (!runtime || !weakScheduler.lock()) {
          if (onComplete) {
            onComplete();
          }
          return;
        }

        jsi::Runtime &rt = worklets::getJSIRuntimeFromWorkletRuntime(runtime);

        auto audioData = jsi::Array(rt, channelCount);
        for (size_t ch = 0; ch < channelCount; ++ch) {
          audioData.setValueAtIndex(rt, ch, jsi::ArrayBuffer(rt, (*channels)[ch]));
        }

        worklets::runSyncOnRuntime(
            runtime,
            serializableWorklet,
            jsi::Value(std::move(audioData)),
            jsi::Value(static_cast<int>(channelCount)));

        if (onComplete) {
          onComplete();
        }
      });
}

} // namespace audioworklets
