#include <algorithm>

#include "AudioArray.h"
#include "AudioBufferSourceNode.h"
#include "AudioBus.h"
#include "AudioParam.h"
#include "AudioUtils.h"
#include "BaseAudioContext.h"
#include "Constants.h"

namespace audioapi {

AudioBufferSourceNode::AudioBufferSourceNode(BaseAudioContext *context)
    : AudioScheduledSourceNode(context),
      loop_(false),
      loopStart_(0),
      loopEnd_(0),
      vReadIndex_(0.0) {
  numberOfInputs_ = 0;
  buffer_ = std::shared_ptr<AudioBuffer>(nullptr);

  detuneParam_ = std::make_shared<AudioParam>(0.0, -MAX_DETUNE, MAX_DETUNE);
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
  if (!buffer) {
    buffer_ = std::shared_ptr<AudioBuffer>(nullptr);
    alignedBus_ = std::shared_ptr<AudioBus>(nullptr);
    return;
  }

  buffer_ = buffer;
  alignedBus_ = std::make_shared<AudioBus>(
      context_->getSampleRate(), buffer_->getLength());

  alignedBus_->zero();
  alignedBus_->sum(buffer_->bus_.get());
}

void AudioBufferSourceNode::processNode(
    AudioBus *processingBus,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  updatePlaybackInfo(processingBus, framesToProcess, startOffset, offsetLength);
  float playbackRate = getPlaybackRateValue(startOffset);

  // No audio data to fill, zero the output and return.
  if (!isPlaying() || !alignedBus_ || alignedBus_->getSize() == 0 ||
      !playbackRate) {
    processingBus->zero();
    return;
  }

  if (std::fabs(playbackRate) == 1.0) {
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
    AudioBus *processingBus,
    size_t startOffset,
    size_t offsetLength,
    float playbackRate) {
  size_t direction = playbackRate < 0 ? -1 : 1;

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
  vReadIndex_ = readIndex;
}

void AudioBufferSourceNode::processWithInterpolation(
    AudioBus *processingBus,
    size_t startOffset,
    size_t offsetLength,
    float playbackRate) {
  size_t direction = playbackRate < 0 ? -1 : 1;

  size_t writeIndex = startOffset;

  double vFrameStart = getVirtualStartFrame();
  double vFrameEnd = getVirtualEndFrame();
  double vFrameDelta = vFrameEnd - vFrameStart;

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
    float factor = vReadIndex_ - readIndex;

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
    vReadIndex_ += playbackRate * direction;
    framesLeft -= 1;

    if (vReadIndex_ < vFrameStart || vReadIndex_ >= vFrameEnd) {
      vReadIndex_ -= direction * vFrameDelta;

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
  double time =
      context_->getCurrentTime() + startOffset / context_->getSampleRate();

  return playbackRateParam_->getValueAtTime(time) *
      std::pow(2.0f, detuneParam_->getValueAtTime(time) / 1200.0f);
}

double AudioBufferSourceNode::getVirtualStartFrame() {
  double loopStartFrame = loopStart_ * context_->getSampleRate();

  return loop_ && loopStartFrame >= 0 && loopStart_ < loopEnd_ ? loopStartFrame
                                                               : 0.0;
}

double AudioBufferSourceNode::getVirtualEndFrame() {
  double inputBufferLength = alignedBus_->getSize();
  double loopEndFrame = loopEnd_ * context_->getSampleRate();

  return loop_ && loopEndFrame > 0 && loopStart_ < loopEnd_
      ? std::min(loopEndFrame, inputBufferLength)
      : inputBufferLength;
}

} // namespace audioapi
