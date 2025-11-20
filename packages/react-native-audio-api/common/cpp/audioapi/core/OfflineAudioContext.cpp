#include "OfflineAudioContext.h"

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/utils/AudioNodeManager.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

#include <algorithm>
#include <cassert>
#include <iostream>
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
    : BaseAudioContext(audioEventHandlerRegistry, runtimeRegistry),
      length_(length),
      numberOfChannels_(numberOfChannels),
      currentSampleFrame_(0) {
  sampleRate_ = sampleRate;
  resultBus_ =
      std::make_shared<AudioBus>(static_cast<int>(length_), numberOfChannels_, sampleRate_);
}

OfflineAudioContext::~OfflineAudioContext() {
  nodeManager_->cleanup();
}

void OfflineAudioContext::resume() {
  Locker locker(mutex_);

  if (state_ == ContextState::RUNNING) {
    return;
  }

  renderAudio();
}

void OfflineAudioContext::suspend(double when, const std::function<void()> &callback) {
  Locker locker(mutex_);

  // we can only suspend once per render quantum at the end of the quantum
  // first quantum is [0, RENDER_QUANTUM_SIZE)
  auto frame = static_cast<size_t>(when * sampleRate_);
  frame = RENDER_QUANTUM_SIZE * ((frame + RENDER_QUANTUM_SIZE - 1) / RENDER_QUANTUM_SIZE);

  if (scheduledSuspends_.find(frame) != scheduledSuspends_.end()) {
    throw std::runtime_error(
        "cannot schedule more than one suspend at frame " + std::to_string(frame) + " (" +
        std::to_string(when) + " seconds)");
  }

  scheduledSuspends_.emplace(frame, callback);
}

void OfflineAudioContext::renderAudio() {
  state_ = ContextState::RUNNING;
  std::thread([this]() {
    auto audioBus = std::make_shared<AudioBus>(RENDER_QUANTUM_SIZE, numberOfChannels_, sampleRate_);

    while (currentSampleFrame_ < length_) {
      Locker locker(mutex_);
      int framesToProcess =
          std::min(static_cast<int>(length_ - currentSampleFrame_), RENDER_QUANTUM_SIZE);

      destination_->renderAudio(audioBus, framesToProcess);

      for (int i = 0; i < framesToProcess; i++) {
        for (int channel = 0; channel < numberOfChannels_; channel += 1) {
          resultBus_->getChannel(channel)->getData()[currentSampleFrame_ + i] =
              audioBus->getChannel(channel)->getData()[i];
        }
      }

      currentSampleFrame_ += framesToProcess;

      // Execute scheduled suspend if exists
      auto suspend = scheduledSuspends_.find(currentSampleFrame_);
      if (suspend != scheduledSuspends_.end()) {
        assert(currentSampleFrame_ < length_);
        auto callback = suspend->second;
        scheduledSuspends_.erase(currentSampleFrame_);
        state_ = ContextState::SUSPENDED;
        callback();
        return;
      }
    }

    // Rendering completed
    auto buffer = std::make_shared<AudioBuffer>(resultBus_);
    resultCallback_(buffer);
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
