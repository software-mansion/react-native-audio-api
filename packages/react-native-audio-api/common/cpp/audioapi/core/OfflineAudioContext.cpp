#include <audioapi/core/OfflineAudioContext.h>

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/utils/AudioArray.hpp>

#include <audioapi/core/types/ContextState.h>
#include <algorithm>
#include <cassert>
#include <memory>
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
  // (consumer) thread. Until then there is no consumer draining the bounded
  // graph-event channel, so a large graph would otherwise fill it and block
  // the producer forever. Let the producing thread drain it itself for now;
  // renderAudio() hands draining back to the render thread.
  getGraph()->enableProducerSelfDrain();
}

OfflineAudioContext::~OfflineAudioContext() {
  // Join the promise worker first: a queued resume() task could spawn a fresh
  // render thread after the join below. Both must be gone before base-class
  // members (graph, disposer, driverMutex_) are destroyed.
  joinPendingPromiseWorker();
  stopRendering_.store(true, std::memory_order_release);
  if (renderThread_.joinable()) {
    if (renderThread_.get_id() == std::this_thread::get_id()) {
      // The render thread can drop the context's last reference itself (a
      // drained event lambda holding the final shared_ptr). Self-join would
      // throw; the thread is already past its render loop, so let it finish
      // unobserved.
      renderThread_.detach();
    } else {
      renderThread_.join();
    }
  }
}

void OfflineAudioContext::resume(const std::shared_ptr<ContextPromiseResolver<void>> &promise) {
  // Control-message body: no locking. Serialized by `scheduleAudioEvent`
  // (sync under `driverMutex_`, or drained on the offline render thread which
  // already holds `driverMutex_` around `processGraph`).
  if (!renderingStarted_ || getState() == ContextState::CLOSED) {
    ContextPromiseResolver<void>::reject(
        promise, "Cannot resume an offline audio context that is not suspended.");
    return;
  }

  if (getState() == ContextState::RUNNING) {
    ContextPromiseResolver<void>::reject(
        promise, "Cannot resume an offline audio context that is already running.");
    return;
  }

  renderAudio(promise);
}

bool OfflineAudioContext::suspend(
    double when,
    const std::shared_ptr<ContextPromiseResolver<void>> &promise) {
  // Control-message body: no locking (see `resume`).
  auto frame = static_cast<size_t>(when * getSampleRate());
  frame = RENDER_QUANTUM_SIZE * ((frame + RENDER_QUANTUM_SIZE - 1) / RENDER_QUANTUM_SIZE);

  if (frame == 0 || frame <= currentSampleFrame_ || frame >= length_) {
    ContextPromiseResolver<void>::reject(
        promise, "The suspend time must be within the remaining render duration.");
    return false;
  }

  if (scheduledSuspends_.contains(frame)) {
    ContextPromiseResolver<void>::reject(
        promise,
        "cannot schedule more than one suspend at frame " + std::to_string(frame) + " (" +
            std::to_string(when) + " seconds)");
    return false;
  }

  scheduledSuspends_.emplace(frame, promise);
  return true;
}

void OfflineAudioContext::renderAudio(
    const std::shared_ptr<ContextPromiseResolver<void>> &resumePromise) {
  getGraph()->disableProducerSelfDrain();

  // first wait for the previous render thread to finish
  // can be in the middle because of suspend and then resume call again
  if (renderThread_.joinable()) {
    renderThread_.join();
  }

  renderThread_ = std::thread([this, resumePromise = resumePromise]() mutable {
    ContextPromiseResolver<void>::resolve(resumePromise);
    // The resolver's callbacks (transitively) own this context — both
    // `startRendering` and the HostObject resume path capture it. Release the
    // reference so the render thread never keeps its own context alive:
    resumePromise.reset();

    while (!stopRendering_.load(std::memory_order_acquire) && currentSampleFrame_ < length_) {
      Locker locker(driverMutex_);
      int framesToProcess =
          std::min(static_cast<int>(length_ - currentSampleFrame_), RENDER_QUANTUM_SIZE);

      audioBuffer_->zero();
      processGraph(audioBuffer_.get(), framesToProcess);

      resultBuffer_->copy(*audioBuffer_, 0, currentSampleFrame_, framesToProcess);

      currentSampleFrame_ += framesToProcess;

      // Execute scheduled suspend if exists
      auto suspend = scheduledSuspends_.find(currentSampleFrame_);
      if (suspend != scheduledSuspends_.end()) {
        assert(currentSampleFrame_ < length_);
        auto promise = std::move(suspend->second);
        scheduledSuspends_.erase(currentSampleFrame_);
        processAudioEvents();
        // The render thread is about to exit; with no consumer again, let the
        // producer self-drain any graph mutations made from the suspend
        // callback until resume() restarts rendering.
        getGraph()->enableProducerSelfDrain();
        locker.unlock();
        ContextPromiseResolver<void>::resolve(promise);
        return;
      }
    }

    if (stopRendering_.load(std::memory_order_acquire)) {
      return;
    }
    OfflineAudioContextResultPromise::resolve(resultPromise_, resultBuffer_);
    resultPromise_.reset();
  });
}

void OfflineAudioContext::startRendering(
    const std::shared_ptr<OfflineAudioContextResultPromise> &promise) {
  if (renderingStarted_) {
    OfflineAudioContextResultPromise::reject(
        promise, "Offline audio rendering has already started.");
    return;
  }

  renderingStarted_ = true;
  resultPromise_ = promise;
  auto runningStatePromise = std::make_shared<ContextPromiseResolver<void>>(
      [self = shared_from_this()]() {
        self->setState(ContextState::RUNNING);
        // startRendering has no acknowledging promise for this transition;
        // the render thread actually starting is the acknowledgment.
        self->dispatchStateChange(ContextState::RUNNING);
      },
      [](const std::string &) {});
  renderAudio(runningStatePromise);
}

bool OfflineAudioContext::isDriverRunning() const {
  return state_.load(std::memory_order_acquire) == ContextState::RUNNING;
}

} // namespace audioapi
