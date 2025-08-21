#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/Constants.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

AudioBufferQueueSourceNode::AudioBufferQueueSourceNode(
    BaseAudioContext *context)
    : AudioBufferBaseSourceNode(context) {
  buffers_ = {};
  stretch_->presetDefault(channelCount_, context_->getSampleRate());

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

void AudioBufferQueueSourceNode::pause() {
  AudioScheduledSourceNode::stop(0.0);
  isPaused_ = true;
}

std::string AudioBufferQueueSourceNode::enqueueBuffer(
    const std::shared_ptr<AudioBuffer> &buffer) {
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

    return;
  }

  AudioScheduledSourceNode::disable();
  buffers_ = {};
}

void AudioBufferQueueSourceNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  if (auto locker = Locker::tryLock(getBufferLock())) {
    // No audio data to fill, zero the output and return.
    if (buffers_.empty()) {
      processingBus->zero();
      return;
    }

    processWithPitchCorrection(processingBus, framesToProcess);

    handleStopScheduled();
  } else {
    processingBus->zero();
  }
}

double AudioBufferQueueSourceNode::getCurrentPosition() const {
  return dsp::sampleFrameToTime(
             static_cast<int>(vReadIndex_), context_->getSampleRate()) +
      playedBuffersDuration_;
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
    assert(writeIndex + framesToCopy <= processingBus->getSize());

    processingBus->copy(
        buffer->bus_.get(), readIndex, writeIndex, framesToCopy);

    writeIndex += framesToCopy;
    readIndex += framesToCopy;
    framesLeft -= framesToCopy;

    if (readIndex >= buffer->getLength()) {
      playedBuffersDuration_ += buffer->getDuration();
      buffers_.pop();

      std::unordered_map<std::string, EventValue> body = {
          {"bufferId", std::to_string(bufferId)}};
      context_->audioEventHandlerRegistry_->invokeHandlerWithEventBody(
          "ended", onEndedCallbackId_, body);

      if (buffers_.empty()) {
        processingBus->zero(writeIndex, framesLeft);
        readIndex = 0;

        break;
      } else {
        data = buffers_.front();
        bufferId = data.first;
        buffer = data.second;

        readIndex = 0;
      }
    }
  }

  // update reading index for next render quantum
  vReadIndex_ = static_cast<double>(readIndex);
}

} // namespace audioapi
