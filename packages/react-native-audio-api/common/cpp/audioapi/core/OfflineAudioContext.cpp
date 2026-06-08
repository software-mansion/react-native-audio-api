#include <audioapi/core/OfflineAudioContext.h>

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/utils/AudioArray.hpp>

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
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const RuntimeRegistry &runtimeRegistry)
    : BaseAudioContext(sampleRate, audioEventHandlerRegistry, runtimeRegistry),
      length_(length),
      currentSampleFrame_(0),
      audioBuffer_(
          std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, numberOfChannels, sampleRate)),
      resultBuffer_(std::make_shared<AudioBuffer>(length, numberOfChannels, sampleRate)) {}

OfflineAudioContext::~OfflineAudioContext() {
  getGraphManager()->cleanup();
}

void OfflineAudioContext::resume() {
  Locker locker(mutex_);

  if (getState() == ContextState::RUNNING) {
    return;
  }

  renderAudio();
}

void OfflineAudioContext::suspend(double when, const std::function<void()> &callback) {
  Locker locker(mutex_);

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

  std::thread([this]() {
    while (currentSampleFrame_ < length_) {
      Locker locker(mutex_);
      int framesToProcess =
          std::min(static_cast<int>(length_ - currentSampleFrame_), RENDER_QUANTUM_SIZE);

      audioBuffer_->zero();
      destination_->renderAudio(audioBuffer_, framesToProcess);

      resultBuffer_->copy(*audioBuffer_, 0, currentSampleFrame_, framesToProcess);

      currentSampleFrame_ += framesToProcess;

      // Execute scheduled suspend if exists
      auto suspend = scheduledSuspends_.find(currentSampleFrame_);
      if (suspend != scheduledSuspends_.end()) {
        assert(currentSampleFrame_ < length_);
        auto callback = suspend->second;
        scheduledSuspends_.erase(currentSampleFrame_);
        setState(ContextState::SUSPENDED);
        callback();
        return;
      }
    }

    // Rendering completed
    resultCallback_(resultBuffer_);
  }).detach();
}

void OfflineAudioContext::startRendering(OfflineAudioContextResultCallback callback) {
  Locker locker(mutex_);

  resultCallback_ = std::move(callback);
  renderAudio();
}

bool OfflineAudioContext::isDriverRunning() const {
  return true;
}

} // namespace audioapi
