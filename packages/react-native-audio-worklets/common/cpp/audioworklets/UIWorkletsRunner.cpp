#include <audioworklets/UIWorkletsRunner.h>
#include <jsi/jsi.h>
#ifdef ANDROID
#include <fbjni/fbjni.h>
#endif
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

  auto channelView = job_->channelView;
  if (channelView == nullptr) {
    return;
  }

  auto uiScheduler = job_->uiScheduler.lock();
  if (uiScheduler == nullptr) {
    channelView->releaseJsValues();
    return;
  }

#ifdef ANDROID
  facebook::jni::ThreadScope::WithClassLoader([uiScheduler, channelView]() {
    worklets::scheduleOnUI(uiScheduler, [channelView]() { channelView->releaseJsValues(); });
  });
#else
  worklets::scheduleOnUI(uiScheduler, [channelView]() { channelView->releaseJsValues(); });
#endif
}

bool UIWorkletsRunner::isActive() const {
  return job_->alive->load(std::memory_order_acquire);
}

void UIWorkletsRunner::createChannelView(size_t frameCount) {
  auto uiRuntime = job_->uiRuntime.lock();
  if (!uiRuntime) {
    return;
  }

  job_->channelView = std::make_shared<AudioChannelViews>(uiRuntime, frameCount, 1);
}

void UIWorkletsRunner::call(std::function<void()> onComplete) const {
  auto uiRuntime = job_->uiRuntime.lock();
  auto uiScheduler = job_->uiScheduler.lock();
  if (!isActive() || !job_->serializableWorklet || !uiRuntime || !uiScheduler) {
    if (onComplete) {
      onComplete();
    }
    return;
  }

  job_->onComplete = std::move(onComplete);

#ifdef ANDROID
  facebook::jni::ThreadScope::WithClassLoader([job = job_, uiScheduler]() {
    worklets::scheduleOnUI(uiScheduler, [job]() { runUIWorkletJob(job); });
  });
#else
  worklets::scheduleOnUI(uiScheduler, [job = job_]() { runUIWorkletJob(job); });
#endif
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
  auto channelView = job->channelView;
  const jsi::Value *audioData =
      channelView != nullptr ? channelView->monoFloat32Channel() : nullptr;
  if (!runtime || !job->uiScheduler.lock() || audioData == nullptr) {
    if (job->onComplete) {
      job->onComplete();
    }
    return;
  }

  worklets::runSyncOnRuntime(runtime, job->serializableWorklet, *audioData);

  if (job->onComplete) {
    job->onComplete();
  }
}

} // namespace audioworklets
