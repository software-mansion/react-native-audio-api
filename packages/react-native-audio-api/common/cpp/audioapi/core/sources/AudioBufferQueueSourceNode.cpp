#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

#include <algorithm>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>

namespace audioapi {

AudioBufferQueueSourceNode::AudioBufferQueueSourceNode(
    std::shared_ptr<BaseAudioContext> context,
    bool pitchCorrection)
    : AudioBufferBaseSourceNode(context, pitchCorrection), buffers_() {
  stretch_->presetDefault(channelCount_, context->getSampleRate());

  if (pitchCorrection) {
    // If pitch correction is enabled, add extra frames at the end
    // to compensate for processing latency.
    addExtraTailFrames_ = true;

    int extraTailFrames = static_cast<int>(stretch_->inputLatency() + stretch_->outputLatency());
    tailBuffer_ =
        std::make_shared<AudioBuffer>(channelCount_, extraTailFrames, context->getSampleRate());

    tailBuffer_->bus_->zero();
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

std::shared_ptr<AudioBus> AudioBufferQueueSourceNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  if (auto locker = Locker::tryLock(getBufferLock())) {
    // no audio data to fill, zero the output and return.
    if (buffers_.empty()) {
      processingBus->zero();
      return processingBus;
    }

    if (!pitchCorrection_) {
      processWithoutPitchCorrection(processingBus, framesToProcess);
    } else {
      processWithPitchCorrection(processingBus, framesToProcess);
    }

    handleStopScheduled();
  } else {
    processingBus->zero();
  }

  return processingBus;
}

double AudioBufferQueueSourceNode::getCurrentPosition() const {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    return dsp::sampleFrameToTime(static_cast<int>(vReadIndex_), context->getSampleRate()) +
        playedBuffersDuration_;
  } else {
    return 0.0;
  }
}

/**
 * Helper functions
 */

void AudioBufferQueueSourceNode::processWithoutInterpolation(
    const std::shared_ptr<AudioBus> &processingBus,
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
    size_t framesToEnd = buffer->getLength() - readIndex;
    size_t framesToCopy = std::min(framesToEnd, framesLeft);
    framesToCopy = framesToCopy > 0 ? framesToCopy : 0;

    assert(readIndex >= 0);
    assert(writeIndex >= 0);
    assert(readIndex + framesToCopy <= buffer->getLength());
    assert(writeIndex + framesToCopy <= processingBus->getSize());

    processingBus->copy(buffer->bus_.get(), readIndex, writeIndex, framesToCopy);

    writeIndex += framesToCopy;
    readIndex += framesToCopy;
    framesLeft -= framesToCopy;

    if (readIndex >= buffer->getLength()) {
      playedBuffersDuration_ += buffer->getDuration();
      buffers_.pop();

      std::unordered_map<std::string, EventValue> body = {
          {"bufferId", std::to_string(bufferId)}, {"isLast", buffers_.empty()}};
      if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
        context->audioEventHandlerRegistry_->invokeHandlerWithEventBody(
            "ended", onEndedCallbackId_, body);
      }

      if (buffers_.empty()) {
        if (addExtraTailFrames_) {
          buffers_.emplace(bufferId_, tailBuffer_);
          bufferId_++;

          addExtraTailFrames_ = false;
        } else if (buffers_.empty()) {
          processingBus->zero(writeIndex, framesLeft);
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
    const std::shared_ptr<AudioBus> &processingBus,
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

    if (nextReadIndex >= buffer->getLength()) {
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

    for (int i = 0; i < processingBus->getNumberOfChannels(); i += 1) {
      float *destination = processingBus->getChannel(i)->getData();
      const float *currentSource = buffer->bus_->getChannel(i)->getData();

      if (crossBufferInterpolation) {
        const float *nextSource = nextBuffer->bus_->getChannel(i)->getData();
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

    if (vReadIndex_ >= static_cast<double>(buffer->getLength())) {
      playedBuffersDuration_ += buffer->getDuration();
      buffers_.pop();

      std::unordered_map<std::string, EventValue> body = {{"bufferId", std::to_string(bufferId)}};
      if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
        context->audioEventHandlerRegistry_->invokeHandlerWithEventBody(
            "ended", onEndedCallbackId_, body);
      }

      if (buffers_.empty()) {
        processingBus->zero(writeIndex, framesLeft);
        vReadIndex_ = 0.0;
        break;
      }

      vReadIndex_ = vReadIndex_ - buffer->getLength();
      data = buffers_.front();
      bufferId = data.first;
      buffer = data.second;
    }
  }
}

} // namespace audioapi
