#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <audioapi/core/utils/buffer/SingleBufferProcessor.h>
#include <algorithm>
#include <cmath>
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
      loopEnd_(options.loopEnd),
      onLoopEndedEvent_(context->getAudioEventHandlerRegistry(), context->getAudioEventProducer()) {
  auto onLoopEnded = [this] {
    sendOnLoopEndedEvent();
  };

  processor_ = std::make_unique<SingleBufferProcessor>(onLoopEnded);
  processor_->setLoop(loop_);
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

  if (isFinished()) {
    return;
  }

  if (buffer_ != nullptr) {
    context->getDisposer()->dispose(std::move(buffer_));
  }

  if (audioBuffer_ != nullptr) {
    context->getDisposer()->dispose(std::move(audioBuffer_));
  }

  buffer_ = buffer;
  audioBuffer_ = audioBuffer;
  processor_->setBuffer(buffer_);
  applyStartToBuffer();
}

void AudioBufferSourceNode::start(double when, double offset, double duration) {
  AudioScheduledSourceNode::start(when);

  // Spec: both wait for a buffer assigned after start().
  startAwaitingBuffer_ = StartArguments{
      .offset = offset,
      .duration =
          duration >= 0 && std::isfinite(duration) ? std::optional(duration) : std::nullopt};
  applyStartToBuffer();
}

void AudioBufferSourceNode::applyStartToBuffer() {
  if (buffer_ == nullptr || !startAwaitingBuffer_) {
    return;
  }
  const auto [offset, duration] = *std::exchange(startAwaitingBuffer_, std::nullopt);
  const auto sampleRate = buffer_->getSampleRate();

  // Past the (effective) loop end, or past the buffer when not looping, playback starts
  // at that boundary instead.
  double startOffset = std::min(offset, getVirtualEndFrame(sampleRate) / sampleRate);
  if (loop_) {
    startOffset = std::min(startOffset, loopEnd_);
  }
  vReadIndex_ = sampleRate * startOffset;

  if (duration) {
    // Spec: `duration` counts buffer content, not context time.
    processor_->setFrameBudget(dsp::timeToSampleFrame(*duration, sampleRate));
  }
}

void AudioBufferSourceNode::disable() {
  AudioScheduledSourceNode::disable();

  if (auto context = context_.lock()) {
    context->getDisposer()->dispose(std::move(buffer_));
  }
  buffer_ = nullptr;
}

void AudioBufferSourceNode::assignOnLoopEndedCallbackId(uint64_t callbackId) {
  onLoopEndedEvent_.assignCallbackId(callbackId);
}

double AudioBufferSourceNode::getCurrentPosition() const {
  return dsp::sampleFrameToTime(static_cast<int>(vReadIndex_), buffer_->getSampleRate());
}

void AudioBufferSourceNode::sendOnLoopEndedEvent() {
  onLoopEndedEvent_.dispatchEmptyFromAudioThread();
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

  if (processor_->atBoundary() && processor_->shouldStop()) {
    playbackState_ = PlaybackState::STOP_SCHEDULED;
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
