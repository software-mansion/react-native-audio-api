#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <algorithm>
#include <memory>

namespace audioapi {

AudioBufferSourceNode::AudioBufferSourceNode(
    std::shared_ptr<BaseAudioContext> context,
    bool pitchCorrection)
    : AudioBufferBaseSourceNode(context, pitchCorrection),
      loop_(false),
      loopSkip_(false),
      loopStart_(0),
      loopEnd_(0),
      buffer_(nullptr),
      alignedBus_(nullptr) {
  isInitialized_ = true;
}

AudioBufferSourceNode::~AudioBufferSourceNode() {
  Locker locker(getBufferLock());

  buffer_.reset();
  alignedBus_.reset();
}

bool AudioBufferSourceNode::getLoop() const {
  return loop_;
}

bool AudioBufferSourceNode::getLoopSkip() const {
  return loopSkip_;
}

double AudioBufferSourceNode::getLoopStart() const {
  return loopStart_;
}

double AudioBufferSourceNode::getLoopEnd() const {
  return loopEnd_;
}

std::shared_ptr<AudioBuffer> AudioBufferSourceNode::getBuffer() const {
  return buffer_;
}

void AudioBufferSourceNode::setLoop(bool loop) {
  loop_ = loop;
}

void AudioBufferSourceNode::setLoopSkip(bool loopSkip) {
  loopSkip_ = loopSkip;
}

void AudioBufferSourceNode::setLoopStart(double loopStart) {
  if (loopSkip_) {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      vReadIndex_ = loopStart * context->getSampleRate();
    }
  }
  loopStart_ = loopStart;
}

void AudioBufferSourceNode::setLoopEnd(double loopEnd) {
  loopEnd_ = loopEnd;
}

void AudioBufferSourceNode::setBuffer(const std::shared_ptr<AudioBuffer> &buffer) {
  Locker locker(getBufferLock());
  std::shared_ptr<BaseAudioContext> context = context_.lock();

  if (buffer == nullptr || context == nullptr) {
    buffer_ = std::shared_ptr<AudioBuffer>(nullptr);
    alignedBus_ = std::shared_ptr<AudioBus>(nullptr);
    loopEnd_ = 0;
    return;
  }

  buffer_ = buffer;
  channelCount_ = buffer_->getNumberOfChannels();

  stretch_->presetDefault(channelCount_, buffer_->getSampleRate());

  if (pitchCorrection_) {
    int extraTailFrames =
        static_cast<int>((getInputLatency() + getOutputLatency()) * context->getSampleRate());
    size_t totalSize = buffer_->getLength() + extraTailFrames;

    alignedBus_ = std::make_shared<AudioBus>(totalSize, channelCount_, buffer_->getSampleRate());
    alignedBus_->copy(buffer_->bus_.get(), 0, 0, buffer_->getLength());

    alignedBus_->zero(buffer_->getLength(), extraTailFrames);
  } else {
    alignedBus_ = std::make_shared<AudioBus>(*buffer_->bus_);
  }
  audioBus_ =
      std::make_shared<AudioBus>(RENDER_QUANTUM_SIZE, channelCount_, context->getSampleRate());
  playbackRateBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE * 3, channelCount_, context->getSampleRate());

  loopEnd_ = buffer_->getDuration();
}

void AudioBufferSourceNode::start(double when, double offset, double duration) {
  AudioScheduledSourceNode::start(when);

  if (duration > 0) {
    AudioScheduledSourceNode::stop(when + duration);
  }

  if (!alignedBus_) {
    return;
  }

  offset =
      std::min(offset, static_cast<double>(alignedBus_->getSize()) / alignedBus_->getSampleRate());

  if (loop_) {
    offset = std::min(offset, loopEnd_);
  }

  vReadIndex_ = static_cast<double>(alignedBus_->getSampleRate() * offset);
}

void AudioBufferSourceNode::disable() {
  AudioScheduledSourceNode::disable();
  alignedBus_.reset();
}

void AudioBufferSourceNode::setOnLoopEndedCallbackId(uint64_t callbackId) {
  auto oldCallbackId = onLoopEndedCallbackId_.exchange(callbackId, std::memory_order_acq_rel);

  if (oldCallbackId != 0) {
    audioEventHandlerRegistry_->unregisterHandler("loopEnded", oldCallbackId);
  }
}

std::shared_ptr<AudioBus> AudioBufferSourceNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  if (auto locker = Locker::tryLock(getBufferLock())) {
    // No audio data to fill, zero the output and return.
    if (!alignedBus_) {
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

double AudioBufferSourceNode::getCurrentPosition() const {
  return dsp::sampleFrameToTime(static_cast<int>(vReadIndex_), buffer_->getSampleRate());
}

void AudioBufferSourceNode::sendOnLoopEndedEvent() {
  auto onLoopEndedCallbackId = onLoopEndedCallbackId_.load(std::memory_order_acquire);
  if (onLoopEndedCallbackId != 0) {
    audioEventHandlerRegistry_->invokeHandlerWithEventBody("loopEnded", onLoopEndedCallbackId, {});
  }
}

/**
 * Helper functions
 */

void AudioBufferSourceNode::processWithoutInterpolation(
    const std::shared_ptr<AudioBus> &processingBus,
    size_t startOffset,
    size_t offsetLength,
    float playbackRate) {
  size_t direction = playbackRate < 0.0f ? -1 : 1;

  auto readIndex = static_cast<size_t>(vReadIndex_);
  size_t writeIndex = startOffset;

  size_t frameStart;
  size_t frameEnd;
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    frameStart = static_cast<size_t>(getVirtualStartFrame(context->getSampleRate()));
    frameEnd = static_cast<size_t>(getVirtualEndFrame(context->getSampleRate()));
  } else {
    processingBus->zero();
    return;
  }
  size_t frameDelta = frameEnd - frameStart;

  size_t framesLeft = offsetLength;

  // if we are moving towards loop, we do nothing because we will achieve it
  // otherwise, we wrap to the start of the loop if necessary
  if (loop_ &&
      ((readIndex >= frameEnd && direction == 1) || (readIndex < frameStart && direction == -1))) {
    readIndex = frameStart +
        (static_cast<int64_t>(readIndex) - static_cast<int64_t>(frameStart)) % frameDelta;
  }

  while (framesLeft > 0) {
    size_t framesToEnd = frameEnd - readIndex;
    size_t framesToCopy = std::min(framesToEnd, framesLeft);
    framesToCopy = framesToCopy > 0 ? framesToCopy : 0;

    assert(readIndex >= 0);
    assert(writeIndex >= 0);
    assert(readIndex + framesToCopy <= alignedBus_->getSize());
    assert(writeIndex + framesToCopy <= processingBus->getSize());

    // Direction is forward, we can normally copy the data
    if (direction == 1) {
      processingBus->copy(alignedBus_.get(), readIndex, writeIndex, framesToCopy);
    } else {
      for (int i = 0; i < framesToCopy; i += 1) {
        for (int j = 0; j < processingBus->getNumberOfChannels(); j += 1) {
          (*processingBus->getChannel(j))[writeIndex + i] =
              (*alignedBus_->getChannel(j))[readIndex - i];
        }
      }
    }

    writeIndex += framesToCopy;
    readIndex += framesToCopy * direction;
    framesLeft -= framesToCopy;

    // if we are moving towards loop, we do nothing because we will achieve it
    // otherwise, we wrap to the start of the loop if necessary
    if ((readIndex >= frameEnd && direction == 1) || (readIndex < frameStart && direction == -1)) {
      readIndex -= direction * frameDelta;

      if (!loop_) {
        processingBus->zero(writeIndex, framesLeft);
        playbackState_ = PlaybackState::STOP_SCHEDULED;
        break;
      }

      sendOnLoopEndedEvent();
    }
  }

  // update reading index for next render quantum
  vReadIndex_ = static_cast<double>(readIndex);
}

void AudioBufferSourceNode::processWithInterpolation(
    const std::shared_ptr<AudioBus> &processingBus,
    size_t startOffset,
    size_t offsetLength,
    float playbackRate) {
  size_t direction = playbackRate < 0.0f ? -1 : 1;

  size_t writeIndex = startOffset;

  double vFrameStart;
  double vFrameEnd;
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    vFrameStart = getVirtualStartFrame(context->getSampleRate());
    vFrameEnd = getVirtualEndFrame(context->getSampleRate());
  } else {
    processingBus->zero();
    return;
  }
  auto vFrameDelta = vFrameEnd - vFrameStart;

  auto frameStart = static_cast<size_t>(vFrameStart);
  auto frameEnd = static_cast<size_t>(vFrameEnd);

  size_t framesLeft = offsetLength;

  // Wrap to the start of the loop if necessary
  if (loop_ && (vReadIndex_ >= vFrameEnd || vReadIndex_ < vFrameStart)) {
    vReadIndex_ = vFrameStart + std::fmod(vReadIndex_ - vFrameStart, vFrameDelta);
  }

  while (framesLeft > 0) {
    auto readIndex = static_cast<size_t>(vReadIndex_);
    size_t nextReadIndex = readIndex + 1;
    auto factor = static_cast<float>(vReadIndex_ - static_cast<double>(readIndex));

    if (nextReadIndex >= frameEnd) {
      nextReadIndex = loop_ ? frameStart : readIndex;
    }

    for (int i = 0; i < processingBus->getNumberOfChannels(); i += 1) {
      float *destination = processingBus->getChannel(i)->getData();
      const float *source = alignedBus_->getChannel(i)->getData();

      destination[writeIndex] = dsp::linearInterpolate(source, readIndex, nextReadIndex, factor);
    }

    writeIndex += 1;
    vReadIndex_ += playbackRate * static_cast<double>(direction);
    framesLeft -= 1;

    if (vReadIndex_ < vFrameStart || vReadIndex_ >= vFrameEnd) {
      vReadIndex_ -= static_cast<double>(direction) * vFrameDelta;

      if (!loop_) {
        processingBus->zero(writeIndex, framesLeft);
        playbackState_ = PlaybackState::STOP_SCHEDULED;
        break;
      }

      sendOnLoopEndedEvent();
    }
  }
}

double AudioBufferSourceNode::getVirtualStartFrame(float sampleRate) {
  auto loopStartFrame = loopStart_ * sampleRate;
  return loop_ && loopStartFrame >= 0 && loopStart_ < loopEnd_ ? loopStartFrame : 0.0;
}

double AudioBufferSourceNode::getVirtualEndFrame(float sampleRate) {
  auto inputBufferLength = static_cast<double>(alignedBus_->getSize());
  auto loopEndFrame = loopEnd_ * sampleRate;

  return loop_ && loopEndFrame > 0 && loopStart_ < loopEnd_
      ? std::min(loopEndFrame, inputBufferLength)
      : inputBufferLength;
}

} // namespace audioapi
