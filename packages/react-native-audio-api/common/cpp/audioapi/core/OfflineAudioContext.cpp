#include "OfflineAudioContext.h"

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/Constants.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/utils/AudioBus.h>
#include <audioapi/core/utils/AudioArray.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/core/utils/AudioNodeManager.h>

#include <cassert>
#include <algorithm>
#include <iostream>

namespace audioapi {

OfflineAudioContext::OfflineAudioContext(
  float sampleRate,
  int32_t numFrames)
  : BaseAudioContext(),
    numFrames_(numFrames) {
  sampleRate_ = sampleRate;
  audioDecoder_ = std::make_shared<AudioDecoder>(sampleRate_);
}

OfflineAudioContext::~OfflineAudioContext() {
  nodeManager_->cleanup();
}

void OfflineAudioContext::resume() {
  state_ = ContextState::RUNNING;
}

void OfflineAudioContext::suspend() {
  state_ = ContextState::SUSPENDED;
}

std::shared_ptr<AudioBuffer> OfflineAudioContext::startRendering() {
  auto resultBus = std::make_shared<AudioBus>(
      static_cast<int>(numFrames_), CHANNEL_COUNT, sampleRate_);
  auto audioBus = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE, CHANNEL_COUNT, sampleRate_);

  int processedFrames = 0;
  while (processedFrames < numFrames_) {
    int framesToProcess = std::min(numFrames_ - processedFrames, RENDER_QUANTUM_SIZE);
    destination_->renderAudio(audioBus, framesToProcess);
    for (int i = 0; i < framesToProcess; i++) {
      for (int channel = 0; channel < CHANNEL_COUNT; channel += 1) {
        resultBus->getChannel(channel)->getData()[processedFrames + i] =
            audioBus->getChannel(channel)->getData()[i];
      }
    }
    processedFrames += framesToProcess;
  }
  
  return std::make_shared<AudioBuffer>(resultBus);
}

} // namespace audioapi
