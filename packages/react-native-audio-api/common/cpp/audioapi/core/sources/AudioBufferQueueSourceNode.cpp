#include <audioapi/types/NodeOptions.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>

#include <algorithm>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>

namespace audioapi {

AudioBufferQueueSourceNode::AudioBufferQueueSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const BaseAudioBufferSourceOptions &options)
    : AudioBufferBaseSourceNode(context, options) {
  buffers_ = {};
  stretch_->presetDefault(channelCount_, context->getSampleRate());

  if (options.pitchCorrection) {
    // If pitch correction is enabled, add extra frames at the end
    // to compensate for processing latency.
    addExtraTailFrames_ = true;
    int extraTailFrames = static_cast<int>(stretch_->inputLatency() + stretch_->outputLatency());
    tailBuffer_ = std::make_shared<AudioBuffer>(channelCount_, extraTailFrames, context->getSampleRate());

    tailBuffer_->zero();
  }

  isInitialized_ = true;
}

AudioBufferQueueSourceNode::~AudioBufferQueueSourceNode() {
  Locker locker(getBufferLock());

  buffers_ = {};
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

  if (buffers_.empty()) {
    return;
  }

  offset = std::min(offset, buffers_.front().second->getDuration());
  vReadIndex_ = static_cast<double>(buffers_.front().second->getSampleRate() * offset);
}

void AudioBufferQueueSourceNode::pause() {
  AudioScheduledSourceNode::stop(0.0);
  isPaused_ = true;
}

std::string AudioBufferQueueSourceNode::enqueueBuffer(const std::shared_ptr<AudioBuffer> &buffer) {
  auto locker = Locker(getBufferLock());
  buffers_.emplace(bufferId_, buffer);

  if (tailBuffer_ != nullptr) {
    addExtraTailFrames_ = true;
  }

  return std::to_string(bufferId_++);
}

void AudioBufferQueueSourceNode::dequeueBuffer(const size_t bufferId) {
  auto locker = Locker(getBufferLock());
  if (buffers_.empty()) {
    return;
  }

  if (buffers_.front().first == bufferId) {
    buffers_.pop();
    vReadIndex_ = 0.0;
    return;
  }

  // If the buffer is not at the front, we need to remove it from the queue.
  // And keep vReadIndex_ at the same position.
  std::queue<std::pair<size_t, std::shared_ptr<AudioBuffer>>> newQueue;
  while (!buffers_.empty()) {
    if (buffers_.front().first != bufferId) {
      newQueue.push(buffers_.front());
    }
    buffers_.pop();
  }
  std::swap(buffers_, newQueue);
}

void AudioBufferQueueSourceNode::clearBuffers() {
  auto locker = Locker(getBufferLock());
  buffers_ = {};
  vReadIndex_ = 0.0;
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
  buffers_ = {};
}

void AudioBufferQueueSourceNode::setOnBufferEndedCallbackId(uint64_t callbackId) {
  auto oldCallbackId = onBufferEndedCallbackId_.exchange(callbackId, std::memory_order_acq_rel);

  if (oldCallbackId != 0) {
    audioEventHandlerRegistry_->unregisterHandler(AudioEvent::BUFFER_ENDED, oldCallbackId);
  }
}

std::shared_ptr<AudioBuffer> AudioBufferQueueSourceNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (auto locker = Locker::tryLock(getBufferLock())) {
    // no audio data to fill, zero the output and return.
    if (buffers_.empty()) {
      processingBuffer->zero();
      return processingBuffer;
    }

    if (!pitchCorrection_) {
      processWithoutPitchCorrection(processingBuffer, framesToProcess);
    } else {
      processWithPitchCorrection(processingBuffer, framesToProcess);
    }

    handleStopScheduled();
  } else {
    processingBuffer->zero();
  }

  return processingBuffer;
}

double AudioBufferQueueSourceNode::getCurrentPosition() const {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    return dsp::sampleFrameToTime(static_cast<int>(vReadIndex_), context->getSampleRate()) +
        playedBuffersDuration_;
  } else {
    return 0.0;
  }
}

void AudioBufferQueueSourceNode::sendOnBufferEndedEvent(size_t bufferId, bool isLastBufferInQueue) {
  auto onBufferEndedCallbackId = onBufferEndedCallbackId_.load(std::memory_order_acquire);

  if (onBufferEndedCallbackId != 0) {
    std::unordered_map<std::string, EventValue> body = {
        {"bufferId", std::to_string(bufferId)}, {"isLastBufferInQueue", isLastBufferInQueue}};

    audioEventHandlerRegistry_->invokeHandlerWithEventBody(
        AudioEvent::BUFFER_ENDED, onBufferEndedCallbackId, body);
  }
}

/**
 * Helper functions
 */

void AudioBufferQueueSourceNode::processWithoutInterpolation(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    size_t startOffset,
    size_t offsetLength,
    float playbackRate) {
  auto readIndex = static_cast<size_t>(vReadIndex_);
  size_t writeIndex = startOffset;

  auto data = buffers_.front();
  auto bufferId = data.first;
  auto buffer = data.second;

  size_t framesLeft = offsetLength;

  while (framesLeft > 0) {
    size_t framesToEnd = buffer->getSize() - readIndex;
    size_t framesToCopy = std::min(framesToEnd, framesLeft);
    framesToCopy = framesToCopy > 0 ? framesToCopy : 0;

    assert(readIndex >= 0);
    assert(writeIndex >= 0);
    assert(readIndex + framesToCopy <= buffer->getSize());
    assert(writeIndex + framesToCopy <= processingBuffer->getSize());

    processingBuffer->copy(*buffer, readIndex, writeIndex, framesToCopy);

    writeIndex += framesToCopy;
    readIndex += framesToCopy;
    framesLeft -= framesToCopy;

    if (readIndex >= buffer->getSize()) {
      playedBuffersDuration_ += buffer->getDuration();
      buffers_.pop();

      if (!(buffers_.empty() && addExtraTailFrames_)) {
        sendOnBufferEndedEvent(bufferId, buffers_.empty());
      }

      if (buffers_.empty()) {
        if (addExtraTailFrames_) {
          buffers_.emplace(bufferId, tailBuffer_);
          addExtraTailFrames_ = false;
        } else {
          processingBuffer->zero(writeIndex, framesLeft);
          readIndex = 0;

          break;
        }
      }

      data = buffers_.front();
      bufferId = data.first;
      buffer = data.second;
      readIndex = 0;
    }
  }

  // update reading index for next render quantum
  vReadIndex_ = static_cast<double>(readIndex);
}

void AudioBufferQueueSourceNode::processWithInterpolation(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    size_t startOffset,
    size_t offsetLength,
    float playbackRate) {
  size_t writeIndex = startOffset;
  size_t framesLeft = offsetLength;

  auto data = buffers_.front();
  auto bufferId = data.first;
  auto buffer = data.second;

  while (framesLeft > 0) {
    auto readIndex = static_cast<size_t>(vReadIndex_);
    size_t nextReadIndex = readIndex + 1;
    auto factor = static_cast<float>(vReadIndex_ - static_cast<double>(readIndex));

    bool crossBufferInterpolation = false;
    std::shared_ptr<AudioBuffer> nextBuffer = nullptr;

    if (nextReadIndex >= buffer->getSize()) {
      if (buffers_.size() > 1) {
        auto tempQueue = buffers_;
        tempQueue.pop();
        nextBuffer = tempQueue.front().second;
        nextReadIndex = 0;
        crossBufferInterpolation = true;
      } else {
        nextReadIndex = readIndex;
      }
    }

    for (size_t i = 0; i < processingBuffer->getNumberOfChannels(); i += 1) {
      const auto destination = processingBuffer->getChannel(i)->span();
      const auto currentSource = buffer->getChannel(i)->span();

      if (crossBufferInterpolation) {
        const auto nextSource = nextBuffer->getChannel(i)->span();
        float currentSample = currentSource[readIndex];
        float nextSample = nextSource[nextReadIndex];
        destination[writeIndex] = currentSample + factor * (nextSample - currentSample);
      } else {
        destination[writeIndex] =
            dsp::linearInterpolate(currentSource, readIndex, nextReadIndex, factor);
      }
    }

    writeIndex += 1;
    // queue source node always use positive playbackRate
    vReadIndex_ += std::abs(playbackRate);
    framesLeft -= 1;

    if (vReadIndex_ >= static_cast<double>(buffer->getSize())) {
      playedBuffersDuration_ += buffer->getDuration();
      buffers_.pop();

      sendOnBufferEndedEvent(bufferId, buffers_.empty());

      if (buffers_.empty()) {
        processingBuffer->zero(writeIndex, framesLeft);
        vReadIndex_ = 0.0;
        break;
      }

      vReadIndex_ = vReadIndex_ - buffer->getSize();
      data = buffers_.front();
      bufferId = data.first;
      buffer = data.second;
    }
  }
}

} // namespace audioapi
