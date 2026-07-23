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
  getGraph()->setProducerSelfDrain(true);
}

void OfflineAudioContext::resume() {
  std::scoped_lock lock(driverMutex_);

  if (getState() == ContextState::RUNNING) {
    return;
  }

  renderAudio();
}

void OfflineAudioContext::suspend(double when, const std::function<void()> &callback) {
  std::scoped_lock lock(driverMutex_);

  // we can only suspend once per render quantum at the end of the quantum
  // first quantum is [0, RENDER_QUANTUM_SIZE)
  auto frame = static_cast<size_t>(when * getSampleRate());
  frame = RENDER_QUANTUM_SIZE * ((frame + RENDER_QUANTUM_SIZE - 1) / RENDER_QUANTUM_SIZE);

  if (scheduledSuspends_.contains(frame)) {
    throw std::runtime_error(
        "cannot schedule more than one suspend at frame " + std::to_string(frame) + " (" +
        std::to_string(when) + " seconds)");
  }

  scheduledSuspends_.emplace(frame, callback);
}

void OfflineAudioContext::renderAudio() {
  setState(ContextState::RUNNING);

  // Flush while we are still the sole consumer, then hand the channel to the
  // render thread.
  getGraph()->processEvents();
  getGraph()->setProducerSelfDrain(false);

  std::thread([this]() {
    while (currentSampleFrame_ < length_) {
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
        auto callback = suspend->second;
        scheduledSuspends_.erase(currentSampleFrame_);
        setState(ContextState::SUSPENDED);
        processAudioEvents();
        // The render thread is about to exit; with no consumer again, let the
        // producer self-drain any graph mutations made from the suspend
        // callback until resume() restarts rendering.
        getGraph()->setProducerSelfDrain(true);
        getGraph()->processEvents();
        locker.unlock();
        callback();
        return;
      }
    }

    // Rendering completed
    resultCallback_(resultBuffer_);
    setState(ContextState::CLOSED);
  }).detach();
}

void OfflineAudioContext::startRendering(OfflineAudioContextResultCallback callback) {
  std::scoped_lock lock(driverMutex_);

  resultCallback_ = std::move(callback);
  renderAudio();
}

bool OfflineAudioContext::isDriverRunning() const {
  return state_.load(std::memory_order_acquire) == ContextState::RUNNING;
}

} // namespace audioapi
