#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/buffer/QueueBufferProcessor.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <algorithm>
#include <memory>
#include <utility>

namespace audioapi {

AudioBufferQueueSourceNode::AudioBufferQueueSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const BaseAudioBufferSourceOptions &options)
    : AudioBufferBaseSourceNode(context, options),
      onBufferEndedEvent_(context->getAudioEventHandlerRegistry()) {
  auto *disposer = context->getDisposer();

  auto onBufferConsumed = [this, disposer](
                              size_t bufferId,
                              std::shared_ptr<AudioBuffer> buffer,
                              bool isLastInQueue,
                              bool fireBufferEndedEvent) {
    playedBuffersDuration_ += buffer->getDuration();
    if (fireBufferEndedEvent) {
      sendOnBufferEndedEvent(bufferId, isLastInQueue);
    }
    disposer->dispose(std::move(buffer));
  };

  processor_ = std::make_unique<QueueBufferProcessor>(&buffers_, onBufferConsumed);
}

void AudioBufferQueueSourceNode::stop(double when) {
  AudioScheduledSourceNode::stop(when);
  isPaused_ = false;
}

void AudioBufferQueueSourceNode::start(double when) {
  isPaused_ = false;
  stopTime_ = -1.0;
  AudioScheduledSourceNode::start(when);
  primeWsolaInput();
}

void AudioBufferQueueSourceNode::start(double when, double offset) {
  isPaused_ = false;
  stopTime_ = -1.0;
  AudioScheduledSourceNode::start(when);

  if (!buffers_.empty() && offset >= 0) {
    offset = std::min(offset, buffers_.front().second->getDuration());
    vReadIndex_ = static_cast<double>(buffers_.front().second->getSampleRate() * offset);
  }

  // Prefill after the start cursor is set so priming matches the offset.
  primeWsolaInput();
}

void AudioBufferQueueSourceNode::resume(double when) {
  // Do not clear endOfStream_ — pause/resume must preserve the EOF signal.
  isPaused_ = false;
  stopTime_ = -1.0;
  AudioScheduledSourceNode::start(when);
}

void AudioBufferQueueSourceNode::pause() {
  AudioScheduledSourceNode::stop(0.0);
  isPaused_ = true;
}

void AudioBufferQueueSourceNode::enqueueBuffer(
    const std::shared_ptr<AudioBuffer> &buffer,
    size_t bufferId) {
  buffers_.emplace_back(bufferId, buffer);
  // More PCM after an EOS mark means the stream continues.
  endOfStream_ = false;
}

void AudioBufferQueueSourceNode::endOfStream() {
  endOfStream_ = true;
  armNaturalEofIfNeeded();
}

void AudioBufferQueueSourceNode::dequeueBuffer(const size_t bufferId) {
  if (auto context = context_.lock()) {
    if (buffers_.empty()) {
      return;
    }

    if (buffers_.front().first == bufferId) {
      context->getDisposer()->dispose(std::move(buffers_.front().second));
      buffers_.pop_front();
      vReadIndex_ = 0.0;
      armNaturalEofIfNeeded();
      return;
    }

    // If the buffer is not at the front, we need to remove it from the linked list..
    // And keep vReadIndex_ at the same position.
    for (auto it = std::next(buffers_.begin()); it != buffers_.end(); ++it) {
      if (it->first == bufferId) {
        context->getDisposer()->dispose(std::move(it->second));
        buffers_.erase(it);
        armNaturalEofIfNeeded();
        return;
      }
    }
  }
}

void AudioBufferQueueSourceNode::clearBuffers() {
  if (auto context = context_.lock()) {
    for (auto it = buffers_.begin(); it != buffers_.end(); ++it) {
      context->getDisposer()->dispose(std::move(it->second));
    }

    buffers_.clear();
    vReadIndex_ = 0.0;
    armNaturalEofIfNeeded();
  }
}

void AudioBufferQueueSourceNode::disable() {
  if (isPaused_) {
    playbackState_ = PlaybackState::UNSCHEDULED;
    startTime_ = -1.0;
    stopTime_ = -1.0;
    isPaused_ = false;

    return;
  }

  AudioScheduledSourceNode::disable();
  clearBuffers();
}

void AudioBufferQueueSourceNode::assignOnBufferEndedCallbackId(uint64_t callbackId) {
  onBufferEndedEvent_.assignCallbackId(callbackId);
}

void AudioBufferQueueSourceNode::setChannelCount(int channelCount) {
  if (channelCount_ != channelCount) {
    channelCount_ = channelCount;
    audioBuffer_ = std::make_shared<DSPAudioBuffer>(
        RENDER_QUANTUM_SIZE, channelCount_, getContextSampleRate());
  }
}

double AudioBufferQueueSourceNode::getCurrentPosition() const {
  return dsp::sampleFrameToTime(static_cast<int>(vReadIndex_), getContextSampleRate()) +
      playedBuffersDuration_;
}

void AudioBufferQueueSourceNode::sendOnBufferEndedEvent(size_t bufferId, bool isLastBufferInQueue) {
  onBufferEndedEvent_.dispatchFromAudioThread(
      BufferEndedPayload{.bufferId = bufferId, .isLastBufferInQueue = isLastBufferInQueue});
}

/**
 * Helper functions
 */

bool AudioBufferQueueSourceNode::isEmpty() const {
  return buffers_.empty();
}

void AudioBufferQueueSourceNode::armNaturalEofIfNeeded() {
  if (!endOfStream_ || !buffers_.empty()) {
    return;
  }
  if (isFinished() || isStopScheduled() || isUnscheduled()) {
    return;
  }
  // Natural EOF (no explicit stopTime_): base processNode drains WSOLA then finishes.
  playbackState_ = PlaybackState::STOP_SCHEDULED;
}

void AudioBufferQueueSourceNode::runBufferProcessor(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    size_t startOffset,
    size_t offsetLength,
    float playbackRate,
    bool interpolate) {
  if (!processingBuffer) {
    return;
  }

  if (buffers_.empty()) {
    processingBuffer->zero(startOffset, offsetLength);
    armNaturalEofIfNeeded();
    return;
  }

  processor_->setPosition(vReadIndex_);
  processor_->process(processingBuffer, startOffset, offsetLength, playbackRate, interpolate);
  vReadIndex_ = processor_->getPosition();

  if (processor_->atBoundary() && processor_->shouldStop()) {
    armNaturalEofIfNeeded();
  }
}

} // namespace audioapi
