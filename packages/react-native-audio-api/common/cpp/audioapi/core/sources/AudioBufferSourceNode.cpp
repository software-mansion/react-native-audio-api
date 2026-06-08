#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <audioapi/core/utils/buffer/SingleBufferProcessor.h>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

namespace audioapi {

AudioBufferSourceNode::AudioBufferSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioBufferSourceOptions &options)
    : AudioBufferBaseSourceNode(context, options),
      loop_(options.loop),
      loopSkip_(options.loopSkip),
      loopStart_(options.loopStart),
      loopEnd_(options.loopEnd) {
  processor_ = std::make_unique<SingleBufferProcessor>();
  isInitialized_.store(true, std::memory_order_release);
}

void AudioBufferSourceNode::setLoop(bool loop) {
  loop_ = loop;
  processor_->setLoop(loop_);
}

void AudioBufferSourceNode::setLoopSkip(bool loopSkip) {
  loopSkip_ = loopSkip;
}

void AudioBufferSourceNode::setLoopStart(double loopStart) {
  if (loopSkip_) {
    vReadIndex_ = loopStart * getContextSampleRate();
  }
  loopStart_ = loopStart;
}

void AudioBufferSourceNode::setLoopEnd(double loopEnd) {
  loopEnd_ = loopEnd;
}

void AudioBufferSourceNode::setBuffer(
    const std::shared_ptr<AudioBuffer> &buffer,
    const std::shared_ptr<DSPAudioBuffer> &audioBuffer) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();

  if (context == nullptr) {
    return;
  }

  auto graphManager = context->getGraphManager();

  if (buffer_ != nullptr) {
    graphManager->addAudioBufferForDestruction(std::move(buffer_));
  }

  // TODO move DSPAudioBuffers destruction to graph manager as well

  if (buffer == nullptr) {
    loopEnd_ = 0;
    channelCount_ = 1;

    buffer_ = nullptr;
    processor_->setBuffer(nullptr);
    return;
  }

  buffer_ = buffer;
  audioBuffer_ = audioBuffer;
  channelCount_ = static_cast<int>(buffer_->getNumberOfChannels());
  loopEnd_ = buffer_->getDuration();
  processor_->setBuffer(buffer_);
}

void AudioBufferSourceNode::start(double when, double offset, double duration) {
  AudioScheduledSourceNode::start(when);

  if (duration > 0) {
    AudioScheduledSourceNode::stop(when + duration);
  }

  if (buffer_ == nullptr) {
    return;
  }

  offset = std::min(offset, static_cast<double>(buffer_->getSize()) / buffer_->getSampleRate());

  if (loop_) {
    offset = std::min(offset, loopEnd_);
  }

  vReadIndex_ = static_cast<double>(buffer_->getSampleRate() * offset);
}

void AudioBufferSourceNode::disable() {
  AudioScheduledSourceNode::disable();
}

void AudioBufferSourceNode::setOnLoopEndedCallbackId(uint64_t callbackId) {
  onLoopEndedCallbackId_ = callbackId;
}

void AudioBufferSourceNode::unregisterOnLoopEndedCallback(uint64_t callbackId) {
  audioEventHandlerRegistry_->unregisterHandler(AudioEvent::LOOP_ENDED, callbackId);
}

double AudioBufferSourceNode::getCurrentPosition() const {
  return dsp::sampleFrameToTime(static_cast<int>(vReadIndex_), buffer_->getSampleRate());
}

void AudioBufferSourceNode::sendOnLoopEndedEvent() {
  if (onLoopEndedCallbackId_ != 0) {
    audioEventHandlerRegistry_->dispatchEvent(
        AudioEvent::LOOP_ENDED, onLoopEndedCallbackId_, EmptyPayload{});
  }
}

/**
 * Helper functions
 */

bool AudioBufferSourceNode::isEmpty() const {
  return buffer_ == nullptr;
}

void AudioBufferSourceNode::runBufferProcessor(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    size_t startOffset,
    size_t offsetLength,
    float playbackRate,
    bool interpolate) {
  if (!processingBuffer) {
    return;
  }

  const float sampleRate = getContextSampleRate();
  const double startFrame = getVirtualStartFrame(sampleRate);
  const double endFrame = getVirtualEndFrame(sampleRate);

  // start(when, offset=duration) sets vReadIndex_ to endFrame, so clamp it
  if (playbackRate < 0 && vReadIndex_ >= endFrame && endFrame > startFrame) {
    vReadIndex_ = endFrame - 1.0;
  }

  processor_->setPosition(vReadIndex_);
  processor_->setEndFrame(static_cast<size_t>(endFrame));
  processor_->setStartFrame(static_cast<size_t>(startFrame));
  processor_->process(processingBuffer, startOffset, offsetLength, playbackRate, interpolate);

  if (processor_->atBoundary()) {
    if (processor_->shouldStop()) {
      playbackState_ = PlaybackState::STOP_SCHEDULED;
    }
    sendOnLoopEndedEvent();
  }

  vReadIndex_ = processor_->getPosition();
}

double AudioBufferSourceNode::getVirtualStartFrame(float sampleRate) const {
  auto loopStartFrame = loopStart_ * sampleRate;
  return loop_ && loopStartFrame >= 0 && loopStart_ < loopEnd_ ? loopStartFrame : 0.0;
}

double AudioBufferSourceNode::getVirtualEndFrame(float sampleRate) {
  auto inputBufferLength = static_cast<double>(buffer_->getSize());
  auto loopEndFrame = loopEnd_ * sampleRate;

  return loop_ && loopEndFrame > 0 && loopStart_ < loopEnd_
      ? std::min(loopEndFrame, inputBufferLength)
      : inputBufferLength;
}

} // namespace audioapi
