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
  /// No lifecycle operation can still be pending here: each one holds a shared_ptr to the
  /// context while queued on the HostObject's promise lane.
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

void OfflineAudioContext::resume(const std::shared_ptr<ContextStateResolver> &promise) {
  // Control-message body: no locking. Serialized by `scheduleAudioEvent`
  // (sync under `driverMutex_`, or drained on the offline render thread which
  // already holds `driverMutex_` around `processGraph`).
  if (!renderingStarted_ || getState() == ContextState::CLOSED) {
    promise->reject(
        "Cannot resume an offline audio context that is not started or already closed.");
    return;
  }

  if (getState() == ContextState::RUNNING) {
    promise->reject("Cannot resume and offline audio context that is already running.");
    return;
  }

  renderAudio(promise);
}

bool OfflineAudioContext::suspend(
    double when,
    const std::shared_ptr<ContextStateResolver> &promise) {
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

  scheduledSuspends_.emplace(frame, promise);
  return true;
}

void OfflineAudioContext::renderAudio(const std::shared_ptr<ContextStateResolver> &resumePromise) {
  getGraph()->disableProducerSelfDrain();

  // first wait for the previous render thread to finish
  // can be in the middle because of suspend and then resume call again
  if (renderThread_.joinable()) {
    renderThread_.join();
  }

  renderThread_ = std::thread([this, resumePromise = resumePromise]() mutable {
    if (resumePromise != nullptr) {
      resumePromise->resolve();
      // The resolver owns this context. Release the reference so the render thread
      // never keeps its own context alive:
      resumePromise.reset();
    } else {
      // startRendering has no promise acknowledging this transition; the render thread
      // actually starting is the acknowledgment.
      setState(ContextState::RUNNING);
      dispatchStateChange(ContextState::RUNNING);
    }

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
        promise->resolve();
        return;
      }
    }

    if (stopRendering_.load(std::memory_order_acquire)) {
      return;
    }
    resultPromise_->resolve(resultBuffer_);
    resultPromise_.reset();
  });
}

void OfflineAudioContext::startRendering(const std::shared_ptr<ContextRenderResolver> &promise) {
  if (renderingStarted_) {
    promise->reject("Offline audio rendering has already started.");
    return;
  }

  renderingStarted_ = true;
  resultPromise_ = promise;
  renderAudio(nullptr);
}

std::shared_ptr<ConvolverThreadPool> OfflineAudioContext::getConvolverThreadPool() {
  return nullptr;
}

bool OfflineAudioContext::isDriverRunning() const {
  return state_.load(std::memory_order_acquire) == ContextState::RUNNING;
}

} // namespace audioapi
