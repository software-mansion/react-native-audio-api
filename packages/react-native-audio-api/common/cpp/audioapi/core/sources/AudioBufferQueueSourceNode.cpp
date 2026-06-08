#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
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
    : AudioBufferBaseSourceNode(context, options) {
  if (options.pitchCorrection) {
    // If pitch correction is enabled, add extra frames at the end
    // to compensate for processing latency.
    addExtraTailFrames_ = true;
  }

  isInitialized_.store(true, std::memory_order_release);

  auto graphManager = context->getGraphManager();

  auto onBufferConsumed = [this, graphManager](
                              size_t bufferId,
                              std::shared_ptr<AudioBuffer> buffer,
                              bool isLastInQueue,
                              bool fireBufferEndedEvent) {
    playedBuffersDuration_ += buffer->getDuration();
    if (fireBufferEndedEvent) {
      sendOnBufferEndedEvent(bufferId, isLastInQueue);
    }
    graphManager->addAudioBufferForDestruction(std::move(buffer));
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
}

void AudioBufferQueueSourceNode::start(double when, double offset) {
  start(when);

  if (buffers_.empty() || offset < 0) {
    return;
  }

  offset = std::min(offset, buffers_.front().second->getDuration());
  vReadIndex_ = static_cast<double>(buffers_.front().second->getSampleRate() * offset);
}

void AudioBufferQueueSourceNode::pause() {
  AudioScheduledSourceNode::stop(0.0);
  isPaused_ = true;
}

void AudioBufferQueueSourceNode::enqueueBuffer(
    const std::shared_ptr<AudioBuffer> &buffer,
    size_t bufferId,
    const std::shared_ptr<AudioBuffer> &tailBuffer) {
  buffers_.emplace_back(bufferId, buffer);

  if (tailBuffer != nullptr) {
    tailBuffer_ = tailBuffer;
  }

  if (tailBuffer_ != nullptr) {
    addExtraTailFrames_ = true;
  }
}

void AudioBufferQueueSourceNode::dequeueBuffer(const size_t bufferId) {
  if (auto context = context_.lock()) {
    if (buffers_.empty()) {
      return;
    }

    auto graphManager = context->getGraphManager();

    if (buffers_.front().first == bufferId) {
      graphManager->addAudioBufferForDestruction(std::move(buffers_.front().second));
      buffers_.pop_front();
      vReadIndex_ = 0.0;
      return;
    }

    // If the buffer is not at the front, we need to remove it from the linked list..
    // And keep vReadIndex_ at the same position.
    for (auto it = std::next(buffers_.begin()); it != buffers_.end(); ++it) {
      if (it->first == bufferId) {
        graphManager->addAudioBufferForDestruction(std::move(it->second));
        buffers_.erase(it);
        return;
      }
    }
  }
}

void AudioBufferQueueSourceNode::clearBuffers() {
  if (auto context = context_.lock()) {
    for (auto it = buffers_.begin(); it != buffers_.end(); ++it) {
      context->getGraphManager()->addAudioBufferForDestruction(std::move(it->second));
    }

    buffers_.clear();
    vReadIndex_ = 0.0;
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

void AudioBufferQueueSourceNode::setOnBufferEndedCallbackId(uint64_t callbackId) {
  onBufferEndedCallbackId_ = callbackId;
}

void AudioBufferQueueSourceNode::unregisterOnBufferEndedCallback(uint64_t callbackId) {
  audioEventHandlerRegistry_->unregisterHandler(AudioEvent::BUFFER_ENDED, callbackId);
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
  if (onBufferEndedCallbackId_ != 0) {
    audioEventHandlerRegistry_->dispatchEvent(
        AudioEvent::BUFFER_ENDED,
        onBufferEndedCallbackId_,
        BufferEndedPayload{.bufferId = bufferId, .isLastBufferInQueue = isLastBufferInQueue});
  }
}

/**
 * Helper functions
 */

bool AudioBufferQueueSourceNode::isEmpty() const {
  return buffers_.empty();
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
    return;
  }

  if (addExtraTailFrames_ && tailBuffer_ != nullptr) {
    processor_->setPendingTail(tailBuffer_);
  }

  processor_->setPosition(vReadIndex_);
  processor_->process(processingBuffer, startOffset, offsetLength, playbackRate, interpolate);

  if (processor_->didConsumeTail()) {
    addExtraTailFrames_ = false;
  }

  vReadIndex_ = processor_->getPosition();
}

} // namespace audioapi
