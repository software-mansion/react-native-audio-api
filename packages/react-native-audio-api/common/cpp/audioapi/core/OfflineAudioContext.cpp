#include <audioapi/core/OfflineAudioContext.h>

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.hpp>

#include <audioapi/core/types/ContextState.h>
#include <algorithm>
#include <cassert>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace audioapi {

OfflineAudioContext::OfflineAudioContext(
    int numberOfChannels,
    size_t length,
    float sampleRate,
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : BaseAudioContext(sampleRate, audioEventHandlerRegistry),
      length_(length),
      currentSampleFrame_(0),
      audioBuffer_(
          std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, numberOfChannels, sampleRate)),
      resultBuffer_(std::make_shared<AudioBuffer>(length, numberOfChannels, sampleRate)) {
  // The graph is built entirely before startRendering() spawns the render
  // (consumer) thread. Until then there is no consumer draining the bounded graph-event channel.
  getGraph()->enableProducerSelfDrain();
}

OfflineAudioContext::~OfflineAudioContext() {
  /// No lifecycle operation can still be pending here: each one holds a shared_ptr to the
  /// context while queued on the HostObject's promise lane.
  if (!renderThread_.joinable()) {
    return;
  }

  if (renderThread_.get_id() == std::this_thread::get_id()) {
    // The render thread drops the context's last reference itself when it releases the
    // result promise. Self-join would throw; the thread is already past its render loop,
    // so let it finish unobserved.
    renderThread_.detach();
    return;
  }

  {
    // Under the mutex, so a render thread is either running or fully slept.
    std::scoped_lock lock(driverMutex_);
    stopRendering_ = true;
  }
  resumed_.notify_one();
  renderThread_.join();
}

void OfflineAudioContext::resume(std::unique_ptr<ContextStateResolver> promise) {
  // Control-message body: no locking. Serialized by `scheduleAudioEvent`
  // (sync under `driverMutex_`, or drained on the offline render thread which
  // already holds `driverMutex_` around `processGraph`).
  if (!renderingStarted_ || getState() == ContextState::CLOSED) {
    promise->reject(
        "Cannot resume an offline audio context that is not started or already closed.");
    return;
  }

  if (getState() == ContextState::RUNNING || pendingResume_ != nullptr) {
    promise->reject("Cannot resume and offline audio context that is already running.");
    return;
  }

  getGraph()->disableProducerSelfDrain();
  pendingResume_ = std::move(promise);
  resumed_.notify_one();
}

bool OfflineAudioContext::suspend(double when, std::unique_ptr<ContextStateResolver> promise) {
  // Control-message body: no locking (see `resume`).
  auto frame = static_cast<size_t>(when * getSampleRate());
  frame = RENDER_QUANTUM_SIZE * ((frame + RENDER_QUANTUM_SIZE - 1) / RENDER_QUANTUM_SIZE);

  if (frame == 0 || frame <= currentSampleFrame_ || frame >= length_) {
    promise->reject("The suspend time must be within the remaining render duration.");
    return false;
  }

  if (scheduledSuspends_.contains(frame)) {
    promise->reject(
        "Cannot schedule more than one suspend at the same frame " + std::to_string(frame) + " (" +
        std::to_string(when) + " seconds)");
    return false;
  }

  scheduledSuspends_.emplace(frame, std::move(promise));
  return true;
}

void OfflineAudioContext::startRendering(std::unique_ptr<ContextRenderResolver> promise) {
  if (renderingStarted_) {
    promise->reject("Offline audio rendering has already started.");
    return;
  }

  renderingStarted_ = true;
  resultPromise_ = std::move(promise);
  getGraph()->disableProducerSelfDrain();
  // startRendering's promise settles only when rendering completes, so the transition to
  // RUNNING is acknowledged here. Done before `driverMutex_` is released, so a resume()
  // queued right behind is rejected instead of leaving a stale resume request.
  setState(ContextState::RUNNING);
  dispatchStateChange(ContextState::RUNNING);
  renderThread_ = std::thread([this] { renderUntilFinished(); });
}

void OfflineAudioContext::renderUntilFinished() {
  while (currentSampleFrame_ < length_) {
    // taken per quantum, so control messages and lifecycle operations can interleave
    std::unique_lock driverLock(driverMutex_);
    if (stopRendering_) {
      return;
    }

    renderQuantum();

    auto suspend = scheduledSuspends_.find(currentSampleFrame_);
    if (suspend == scheduledSuspends_.end()) {
      continue;
    }

    assert(currentSampleFrame_ < length_);
    auto suspendPromise = std::move(suspend->second);
    scheduledSuspends_.erase(suspend);
    processAudioEvents();
    // Nothing consumes graph events while suspended, so let the producer self-drain the
    // mutations made from the suspend callback until resume() hands draining back.
    getGraph()->enableProducerSelfDrain();
    // Resolved under `driverMutex_`, so a resume() answering this suspend cannot run until
    // the suspended state and its `statechange` are both out.
    suspendPromise->resolve();
    suspendPromise.reset();

    waitForResume(driverLock);
  }

  resultPromise_->resolve(resultBuffer_);
  // The resolver owns this context, so this may destroy it: nothing may follow.
  resultPromise_.reset();
}

void OfflineAudioContext::renderQuantum() {
  int framesToProcess =
      std::min(static_cast<int>(length_ - currentSampleFrame_), RENDER_QUANTUM_SIZE);

  audioBuffer_->zero();
  processGraph(audioBuffer_.get(), framesToProcess);
  resultBuffer_->copy(*audioBuffer_, 0, currentSampleFrame_, framesToProcess);

  currentSampleFrame_ += framesToProcess;
}

void OfflineAudioContext::waitForResume(std::unique_lock<std::mutex> &driverLock) {
  resumed_.wait(driverLock, [this] { return pendingResume_ != nullptr || stopRendering_; });
  if (pendingResume_ != nullptr) {
    // The render thread picking the request up is the acknowledgment of the resume.
    auto resumePromise = std::move(pendingResume_);
    resumePromise->resolve();
  }
}

std::shared_ptr<ConvolverThreadPool> OfflineAudioContext::getConvolverThreadPool() {
  return nullptr;
}

bool OfflineAudioContext::isDriverRunning() const {
  return state_.load(std::memory_order_acquire) == ContextState::RUNNING;
}

} // namespace audioapi
