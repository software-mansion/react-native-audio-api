#include <audioworklets/UIWorkletsRunner.h>
#include <jsi/jsi.h>

#include <memory>
#include <utility>

namespace audioworklets {

UIWorkletsRunner::UIWorkletsRunner(
    const std::shared_ptr<worklets::WorkletRuntime> &uiRuntime,
    const std::shared_ptr<worklets::UIScheduler> &uiScheduler,
    std::shared_ptr<worklets::Serializable> serializableWorklet)
    : job_(std::make_shared<UIWorkletJob>()) {
  job_->alive = std::make_shared<std::atomic<bool>>(true);
  job_->uiRuntime = uiRuntime;
  job_->uiScheduler = uiScheduler;
  job_->serializableWorklet = std::move(serializableWorklet);
}

void UIWorkletsRunner::deactivate() {
  job_->alive->store(false, std::memory_order_release);

  auto channelViews = job_->channelViews;
  if (channelViews == nullptr) {
    return;
  }

  auto uiScheduler = job_->uiScheduler.lock();
  if (uiScheduler == nullptr) {
    channelViews->releaseJsValues();
    return;
  }

  worklets::scheduleOnUI(uiScheduler, [channelViews]() { channelViews->releaseJsValues(); });
}

bool UIWorkletsRunner::isActive() const {
  return job_->alive->load(std::memory_order_acquire);
}

void UIWorkletsRunner::createChannelViews(size_t frameCount, size_t channelCount) {
  auto uiRuntime = job_->uiRuntime.lock();
  if (!uiRuntime) {
    return;
  }

  job_->channelViews = std::make_shared<AudioChannelViews>(uiRuntime, frameCount, channelCount);
}

void UIWorkletsRunner::call(size_t channelCount, std::function<void()> onComplete) const {
  auto uiRuntime = job_->uiRuntime.lock();
  auto uiScheduler = job_->uiScheduler.lock();
  if (!isActive() || !job_->serializableWorklet || !uiRuntime || !uiScheduler) {
    if (onComplete) {
      onComplete();
    }
    return;
  }

  job_->channelCount = channelCount;
  job_->onComplete = std::move(onComplete);

  worklets::scheduleOnUI(uiScheduler, [job = job_]() { runUIWorkletJob(job); });
}

void UIWorkletsRunner::runUIWorkletJob(const std::shared_ptr<UIWorkletJob> &job) {
  if (job == nullptr) {
    return;
  }

  if (!job->alive->load(std::memory_order_acquire)) {
    if (job->onComplete) {
      job->onComplete();
    }
    return;
  }

  auto runtime = job->uiRuntime.lock();
  auto channelViews = job->channelViews;
  const jsi::Value *audioData =
      channelViews != nullptr ? channelViews->channelsArray(job->channelCount) : nullptr;
  if (!runtime || !job->uiScheduler.lock() || audioData == nullptr) {
    if (job->onComplete) {
      job->onComplete();
    }
    return;
  }

  worklets::runSyncOnRuntime(
      runtime,
      job->serializableWorklet,
      *audioData,
      jsi::Value(static_cast<int>(job->channelCount)));

  if (job->onComplete) {
    job->onComplete();
  }
}

} // namespace audioworklets
