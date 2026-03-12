#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
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

double AudioBufferQueueSourceNode::getCurrentPosition() const {
  return dsp::sampleFrameToTime(static_cast<int>(vReadIndex_), getContextSampleRate()) +
      playedBuffersDuration_;
}

void AudioBufferQueueSourceNode::sendOnBufferEndedEvent(size_t bufferId, bool isLastBufferInQueue) {
  if (onBufferEndedCallbackId_ != 0) {
    std::unordered_map<std::string, EventValue> body = {
        {"bufferId", std::to_string(bufferId)}, {"isLastBufferInQueue", isLastBufferInQueue}};

    audioEventHandlerRegistry_->invokeHandlerWithEventBody(
        AudioEvent::BUFFER_ENDED, onBufferEndedCallbackId_, body);
  }
}

/**
 * Helper functions
 */

bool AudioBufferQueueSourceNode::isEmpty() const {
  return buffers_.empty();
}

void AudioBufferQueueSourceNode::processWithoutInterpolation(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    size_t startOffset,
    size_t offsetLength,
    float playbackRate) {
  if (auto context = context_.lock()) {
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
        buffers_.pop_front();

        if (!(buffers_.empty() && addExtraTailFrames_)) {
          sendOnBufferEndedEvent(bufferId, buffers_.empty());
        }

        if (buffers_.empty()) {
          if (addExtraTailFrames_) {
            buffers_.emplace_back(bufferId, tailBuffer_);
            addExtraTailFrames_ = false;
          } else {
            context->getGraphManager()->addAudioBufferForDestruction(std::move(buffer));
            processingBuffer->zero(writeIndex, framesLeft);
            readIndex = 0;

            break;
          }
        }

        context->getGraphManager()->addAudioBufferForDestruction(std::move(buffer));
        data = buffers_.front();
        bufferId = data.first;
        buffer = data.second;
        readIndex = 0;
      }
    }

    // update reading index for next render quantum
    vReadIndex_ = static_cast<double>(readIndex);
  }
}

void AudioBufferQueueSourceNode::processWithInterpolation(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    size_t startOffset,
    size_t offsetLength,
    float playbackRate) {
  if (auto context = context_.lock()) {
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
          tempQueue.pop_front();
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
        buffers_.pop_front();

        sendOnBufferEndedEvent(bufferId, buffers_.empty());

        if (buffers_.empty()) {
          context->getGraphManager()->addAudioBufferForDestruction(std::move(buffer));
          processingBuffer->zero(writeIndex, framesLeft);
          vReadIndex_ = 0.0;
          break;
        }

        vReadIndex_ = vReadIndex_ - buffer->getSize();
        context->getGraphManager()->addAudioBufferForDestruction(std::move(buffer));
        data = buffers_.front();
        bufferId = data.first;
        buffer = data.second;
      }
    }
  }
}

} // namespace audioapi
