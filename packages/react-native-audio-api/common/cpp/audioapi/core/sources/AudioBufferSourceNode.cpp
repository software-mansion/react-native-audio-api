#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/Constants.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/utils/AudioArray.h>
#include <audioapi/core/utils/AudioBus.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/AudioUtils.h>

namespace audioapi {

AudioBufferSourceNode::AudioBufferSourceNode(BaseAudioContext *context)
    : AudioScheduledSourceNode(context),
      loop_(false),
      loopStart_(0),
      loopEnd_(0),
      vReadIndex_(0.0) {
  buffer_ = std::shared_ptr<AudioBuffer>(nullptr);
  alignedBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE, 1, context_->getSampleRate());

  detuneParam_ = std::make_shared<AudioParam>(0.0, MIN_DETUNE, MAX_DETUNE);
  playbackRateParam_ = std::make_shared<AudioParam>(
      1.0, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT);

  isInitialized_ = true;
}

bool AudioBufferSourceNode::getLoop() const {
  return loop_;
}

double AudioBufferSourceNode::getLoopStart() const {
  return loopStart_;
}

double AudioBufferSourceNode::getLoopEnd() const {
  return loopEnd_;
}

std::shared_ptr<AudioParam> AudioBufferSourceNode::getDetuneParam() const {
  return detuneParam_;
}

std::shared_ptr<AudioParam> AudioBufferSourceNode::getPlaybackRateParam()
    const {
  return playbackRateParam_;
}

std::shared_ptr<AudioBuffer> AudioBufferSourceNode::getBuffer() const {
  return buffer_;
}

void AudioBufferSourceNode::setLoop(bool loop) {
  loop_ = loop;
}

void AudioBufferSourceNode::setLoopStart(double loopStart) {
  loopStart_ = loopStart;
}

void AudioBufferSourceNode::setLoopEnd(double loopEnd) {
  loopEnd_ = loopEnd;
}

void AudioBufferSourceNode::setBuffer(
    const std::shared_ptr<AudioBuffer> &buffer) {
  Locker locker(getBufferLock());

  if (!buffer) {
    buffer_ = std::shared_ptr<AudioBuffer>(nullptr);
    alignedBus_ = std::make_shared<AudioBus>(
        RENDER_QUANTUM_SIZE, 1, context_->getSampleRate());
    loopEnd_ = 0;
    return;
  }

  buffer_ = buffer;
  channelCount_ = buffer_->getNumberOfChannels();

  alignedBus_ = std::make_shared<AudioBus>(
      buffer_->getLength(), channelCount_, context_->getSampleRate());
  alignedBus_->zero();
  alignedBus_->sum(buffer_->bus_.get());

  audioBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE, channelCount_, context_->getSampleRate());

  loopEnd_ = buffer_->getDuration();
}

void AudioBufferSourceNode::start(double when, double offset, double duration) {
  AudioScheduledSourceNode::start(when);

  if (duration > 0) {
    AudioScheduledSourceNode::stop(when + duration);
  }

  if (!buffer_) {
    return;
  }

  offset = std::min(offset, buffer_->getDuration());

  if (loop_) {
    offset = std::min(offset, loopEnd_);
  }

  vReadIndex_ = static_cast<double>(buffer_->getSampleRate() * offset);
}

std::mutex &AudioBufferSourceNode::getBufferLock() {
  return bufferLock_;
}

void AudioBufferSourceNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  // No audio data to fill, zero the output and return.
  if (!buffer_) {
    processingBus->zero();
    return;
  }

  if (!Locker::tryLock(getBufferLock())) {
    processingBus->zero();
    return;
  }

  size_t startOffset = 0;
  size_t offsetLength = 0;

  updatePlaybackInfo(processingBus, framesToProcess, startOffset, offsetLength);
  float playbackRate = getPlaybackRateValue(startOffset);

  assert(alignedBus_ != nullptr);
  assert(alignedBus_->getSize() > 0);

  if (playbackRate == 0.0f || !isPlaying()) {
    processingBus->zero();
    return;
  } else if (std::fabs(playbackRate) == 1.0) {
    processWithoutInterpolation(
        processingBus, startOffset, offsetLength, playbackRate);
  } else {
    processWithInterpolation(
        processingBus, startOffset, offsetLength, playbackRate);
  }

  handleStopScheduled();
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

  auto frameStart = static_cast<size_t>(getVirtualStartFrame());
  auto frameEnd = static_cast<size_t>(getVirtualEndFrame());
  size_t frameDelta = frameEnd - frameStart;

  size_t framesLeft = offsetLength;

  if (loop_ && (readIndex >= frameEnd || readIndex < frameStart)) {
    readIndex = frameStart + (readIndex - frameStart) % frameDelta;
  }

  while (framesLeft > 0) {
    size_t framesToEnd = frameEnd - readIndex;
    size_t framesToCopy = std::min(framesToEnd, framesLeft);
    framesToCopy = framesToCopy > 0 ? framesToCopy : 0;

    // Direction is forward, we can normally copy the data
    if (direction == 1) {
      processingBus->copy(
          alignedBus_.get(), readIndex, writeIndex, framesToCopy);
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

    if (readIndex >= frameEnd || readIndex < frameStart) {
      readIndex -= direction * frameDelta;

      if (!loop_) {
        processingBus->zero(writeIndex, framesLeft);
        playbackState_ = PlaybackState::FINISHED;
        disable();
        break;
      }
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

  auto vFrameStart = getVirtualStartFrame();
  auto vFrameEnd = getVirtualEndFrame();
  auto vFrameDelta = vFrameEnd - vFrameStart;

  auto frameStart = static_cast<size_t>(vFrameStart);
  auto frameEnd = static_cast<size_t>(vFrameEnd);

  size_t framesLeft = offsetLength;

  // Wrap to the start of the loop if necessary
  if (loop_ && (vReadIndex_ >= vFrameEnd || vReadIndex_ < vFrameStart)) {
    vReadIndex_ =
        vFrameStart + std::fmod(vReadIndex_ - vFrameStart, vFrameDelta);
  }

  while (framesLeft > 0) {
    auto readIndex = static_cast<size_t>(vReadIndex_);
    size_t nextReadIndex = readIndex + 1;
    auto factor =
        static_cast<float>(vReadIndex_ - static_cast<double>(readIndex));

    if (nextReadIndex >= frameEnd) {
      nextReadIndex = loop_ ? frameStart : readIndex;
    }

    for (int i = 0; i < processingBus->getNumberOfChannels(); i += 1) {
      float *destination = processingBus->getChannel(i)->getData();
      const float *source = alignedBus_->getChannel(i)->getData();

      destination[writeIndex] = AudioUtils::linearInterpolate(
          source, readIndex, nextReadIndex, factor);
    }

    writeIndex += 1;
    vReadIndex_ += playbackRate * static_cast<double>(direction);
    framesLeft -= 1;

    if (vReadIndex_ < vFrameStart || vReadIndex_ >= vFrameEnd) {
      vReadIndex_ -= static_cast<double>(direction) * vFrameDelta;

      if (!loop_) {
        processingBus->zero(writeIndex, framesLeft);
        playbackState_ = PlaybackState::FINISHED;
        disable();
        break;
      }
    }
  }
}

float AudioBufferSourceNode::getPlaybackRateValue(size_t &startOffset) {
  auto time = context_->getCurrentTime() +
      static_cast<double>(startOffset) / context_->getSampleRate();

  auto sampleRateFactor = buffer_->getSampleRate() / context_->getSampleRate();
  auto detune = std::pow(2.0f, detuneParam_->getValueAtTime(time) / 1200.0f);
  auto playbackRate = playbackRateParam_->getValueAtTime(time);

  return playbackRate * sampleRateFactor * detune;
}

double AudioBufferSourceNode::getVirtualStartFrame() {
  auto loopStartFrame = loopStart_ * context_->getSampleRate();

  return loop_ && loopStartFrame >= 0 && loopStart_ < loopEnd_ ? loopStartFrame
                                                               : 0.0;
}

double AudioBufferSourceNode::getVirtualEndFrame() {
  auto inputBufferLength = static_cast<double>(alignedBus_->getSize());
  auto loopEndFrame = loopEnd_ * context_->getSampleRate();

  return loop_ && loopEndFrame > 0 && loopStart_ < loopEnd_
      ? std::min(loopEndFrame, inputBufferLength)
      : inputBufferLength;
}

} // namespace audioapi
