#include <audioworklets/UIWorkletsRunner.h>
#include <jsi/jsi.h>

#include <memory>
#include <utility>

namespace audioworklets {

UIWorkletsRunner::UIWorkletsRunner(
    const std::shared_ptr<worklets::WorkletRuntime> &uiRuntime,
    const std::shared_ptr<worklets::UIScheduler> &uiScheduler,
    std::shared_ptr<worklets::Serializable> serializableWorklet)
    : alive_(std::make_shared<std::atomic<bool>>(true)),
      uiRuntime_(uiRuntime),
      uiScheduler_(uiScheduler),
      serializableWorklet_(std::move(serializableWorklet)),
      job_(std::make_shared<UIWorkletJob>()) {
  job_->alive = alive_;
  job_->uiRuntime = uiRuntime_;
  job_->uiScheduler = uiScheduler_;
  job_->serializableWorklet = serializableWorklet_;
}

void UIWorkletsRunner::deactivate() {
  alive_->store(false, std::memory_order_release);

  auto channelViews = channelViews_;
  if (channelViews == nullptr) {
    return;
  }

  auto uiScheduler = uiScheduler_.lock();
  if (uiScheduler == nullptr) {
    channelViews->releaseJsValues();
    return;
  }

  worklets::scheduleOnUI(uiScheduler, [channelViews]() { channelViews->releaseJsValues(); });
}

bool UIWorkletsRunner::isActive() const {
  return alive_->load(std::memory_order_acquire);
}

std::shared_ptr<AudioChannelViews> UIWorkletsRunner::createChannelViews(
    size_t frameCount,
    size_t channelCount) {
  auto uiRuntime = uiRuntime_.lock();
  if (!uiRuntime) {
    return nullptr;
  }

  channelViews_ = std::make_shared<AudioChannelViews>(uiRuntime, frameCount, channelCount);
  job_->channelViews = channelViews_;
  return channelViews_;
}

void UIWorkletsRunner::call(size_t channelCount, std::function<void()> onComplete) const {
  auto uiRuntime = uiRuntime_.lock();
  auto uiScheduler = uiScheduler_.lock();
  if (!isActive() || !serializableWorklet_ || !uiRuntime || !uiScheduler) {
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
