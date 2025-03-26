#include "OfflineAudioContext.h"

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/Constants.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/utils/AudioArray.h>
#include <audioapi/core/utils/AudioBus.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/core/utils/AudioNodeManager.h>
#include <audioapi/core/utils/Locker.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>

namespace audioapi {

OfflineAudioContext::OfflineAudioContext(float sampleRate, int32_t numFrames)
    : BaseAudioContext(),
      isRenderingStarted_(false),
      numFrames_(numFrames),
      currentSampleFrame_(0) {
  sampleRate_ = sampleRate;
  audioDecoder_ = std::make_shared<AudioDecoder>(sampleRate_);
}

OfflineAudioContext::~OfflineAudioContext() {
  nodeManager_->cleanup();
}

void OfflineAudioContext::resume() {
  Locker locker(stateLock_);

  if (!isRenderingStarted_) {
    throw std::runtime_error(
        "cannot resume an offline context that has not started");
  }

  if (state_ == ContextState::RUNNING)
    return;
  assert(state_ == ContextState::SUSPENDED);

  resumeRendering();
}

void OfflineAudioContext::suspend(double when, std::function<void()> callback) {
  Locker locker(stateLock_);

  if (when < 0) {
    throw std::runtime_error(
        "negative suspend time (" + std::to_string(when) + ") is not allowed.");
    return;
  }

  double totalRenderDuration = numFrames_ / (double)sampleRate_;
  if (totalRenderDuration <= when) {
    throw std::runtime_error(
        "cannot schedule a suspend at " + std::to_string(when) +
        " seconds becuase it is greater than " + "or equal to the total " +
        "render duration of " + std::to_string(numFrames_) + " frames (" +
        std::to_string(totalRenderDuration) + " seconds)");
  }

  int32_t frame = when * sampleRate_;
  frame = RENDER_QUANTUM_SIZE *
      ((frame + RENDER_QUANTUM_SIZE - 1) / RENDER_QUANTUM_SIZE);

  if (frame < currentSampleFrame_) {
    int32_t currentFrameClamped = std::min(currentSampleFrame_, numFrames_);
    double currentTime = currentSampleFrame_ / (double)sampleRate_;
    double totalTime = numFrames_ / (double)sampleRate_;
    double currentTimeClamped = std::min(currentTime, totalTime);
    throw std::runtime_error(
        "suspend(" + std::to_string(when) + ") failed to suspend at frame " +
        std::to_string(frame) + " becuase it is earlier than the current " +
        "frame of " + std::to_string(currentFrameClamped) + " (" +
        std::to_string(currentTimeClamped) + " seconds");
  }

  if (scheduledSuspends_.find(frame) != scheduledSuspends_.end()) {
    throw std::runtime_error(
        "cannot schedule more than one suspend at frame " +
        std::to_string(frame) + " (" + std::to_string(when) + " seconds)");
  }

  scheduledSuspends_.emplace(frame, callback);
}

void OfflineAudioContext::resumeRendering() {
  state_ = ContextState::RUNNING;
  std::thread([this]() {
    auto audioBus = std::make_shared<AudioBus>(
        RENDER_QUANTUM_SIZE, CHANNEL_COUNT, sampleRate_);

    while (currentSampleFrame_ < numFrames_) {
      Locker locker(stateLock_);
      int framesToProcess =
          std::min(numFrames_ - currentSampleFrame_, RENDER_QUANTUM_SIZE);
      destination_->renderAudio(audioBus, framesToProcess);
      for (int i = 0; i < framesToProcess; i++) {
        for (int channel = 0; channel < CHANNEL_COUNT; channel += 1) {
          resultBus_->getChannel(channel)->getData()[currentSampleFrame_ + i] =
              audioBus->getChannel(channel)->getData()[i];
        }
      }
      currentSampleFrame_ += framesToProcess;

      // Execute scheduled suspend if exists
      auto suspend = scheduledSuspends_.find(currentSampleFrame_);
      if (suspend != scheduledSuspends_.end()) {
        assert(currentSampleFrame_ < numFrames_);
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

void OfflineAudioContext::startRendering(
    OfflineAudioContextResultCallback callback) {
  Locker locker(stateLock_);
  if (isRenderingStarted_) {
    throw std::runtime_error("cannot call startRendering more than once");
  }
  resultBus_ = std::make_shared<AudioBus>(
      static_cast<int>(numFrames_), CHANNEL_COUNT, sampleRate_);
  isRenderingStarted_ = true;
  resultCallback_ = callback;
  resumeRendering();
}

} // namespace audioapi
