#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/Constants.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

AudioBufferSourceNode::AudioBufferSourceNode(
    BaseAudioContext *context,
    bool pitchCorrection)
    : AudioScheduledSourceNode(context),
      loop_(false),
      loopStart_(0),
      loopEnd_(0),
      pitchCorrection_(pitchCorrection),
      vReadIndex_(0.0) {
  buffer_ = std::shared_ptr<AudioBuffer>(nullptr);
  alignedBus_ = std::shared_ptr<AudioBus>(nullptr);

  detuneParam_ = std::make_shared<AudioParam>(
      0.0, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT);
  playbackRateParam_ = std::make_shared<AudioParam>(
      1.0, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT);

  playbackRateBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE * 3, channelCount_, context_->getSampleRate());

  stretch_ =
      std::make_shared<signalsmith::stretch::SignalsmithStretch<float>>();

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
    alignedBus_ = std::shared_ptr<AudioBus>(nullptr);
    loopEnd_ = 0;
    return;
  }

  buffer_ = buffer;
  alignedBus_ = std::make_shared<AudioBus>(*buffer_->bus_);
  channelCount_ = buffer_->getNumberOfChannels();

  audioBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE, channelCount_, context_->getSampleRate());
  playbackRateBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE * 3, channelCount_, context_->getSampleRate());

  loopEnd_ = buffer_->getDuration();

  stretch_->presetDefault(channelCount_, buffer_->getSampleRate());
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

void AudioBufferSourceNode::disable() {
  AudioNode::disable();

  if (onendedCallback_) {
    onendedCallback_(getStopTime());
  }

  alignedBus_.reset();
}

std::mutex &AudioBufferSourceNode::getBufferLock() {
  return bufferLock_;
}

void AudioBufferSourceNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  // No audio data to fill, zero the output and return.
  if (!buffer_ || !alignedBus_) {
    processingBus->zero();
    return;
  }

  if (!Locker::tryLock(getBufferLock())) {
    processingBus->zero();
    return;
  }

  if (!pitchCorrection_) {
    processWithoutPitchCorrection(processingBus, framesToProcess);
  } else {
    processWithPitchCorrection(processingBus, framesToProcess);
  }

  handleStopScheduled();
}

double AudioBufferSourceNode::getStopTime() const {
  return dsp::sampleFrameToTime(
      static_cast<int>(vReadIndex_), buffer_->getSampleRate());
}

/**
 * Helper functions
 */

void AudioBufferSourceNode::processWithoutPitchCorrection(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  auto computedPlaybackRate = getComputedPlaybackRateValue();
  updatePlaybackInfo(processingBus, framesToProcess, startOffset, offsetLength);

  if (computedPlaybackRate == 0.0f || !isPlaying()) {
    processingBus->zero();
    return;
  }

  if (std::fabs(computedPlaybackRate) == 1.0) {
    processWithoutInterpolation(
        processingBus, startOffset, offsetLength, computedPlaybackRate);
  } else {
    processWithInterpolation(
        processingBus, startOffset, offsetLength, computedPlaybackRate);
  }
}

void AudioBufferSourceNode::processWithPitchCorrection(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  auto time = context_->getCurrentTime();
  auto playbackRate =
      std::clamp(playbackRateParam_->getValueAtTime(time), 0.0f, 3.0f);
  auto detune =
      std::clamp(detuneParam_->getValueAtTime(time) / 100.0f, -12.0f, 12.0f);

  playbackRateBus_->zero();

  auto framesNeededToStretch =
      static_cast<int>(playbackRate * static_cast<float>(framesToProcess));

  updatePlaybackInfo(
      playbackRateBus_, framesNeededToStretch, startOffset, offsetLength);

  if (playbackRate == 0.0f || !isPlaying()) {
    processingBus->zero();
    return;
  }

  processWithoutInterpolation(
      playbackRateBus_, startOffset, offsetLength, playbackRate);

  stretch_->process(
      playbackRateBus_.get()[0],
      framesNeededToStretch,
      processingBus.get()[0],
      framesToProcess);

  if (detune != 0.0f) {
    stretch_->setTransposeSemitones(detune);
  }
}

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
        playbackState_ = PlaybackState::STOP_SCHEDULED;
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

      destination[writeIndex] =
          dsp::linearInterpolate(source, readIndex, nextReadIndex, factor);
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
    }
  }
}

float AudioBufferSourceNode::getComputedPlaybackRateValue() {
  auto time = context_->getCurrentTime();

  auto sampleRateFactor = buffer_->getSampleRate() / context_->getSampleRate();
  auto playbackRate = playbackRateParam_->getValueAtTime(time);
  auto detune = std::pow(2.0f, detuneParam_->getValueAtTime(time) / 1200.0f);

  return playbackRate * sampleRateFactor * detune;
}

double AudioBufferSourceNode::getVirtualStartFrame() {
  auto loopStartFrame = loopStart_ * context_->getSampleRate();

  return loop_ && loopStartFrame >= 0 && loopStart_ < loopEnd_ ? loopStartFrame
                                                               : 0.0;
}

double AudioBufferSourceNode::getVirtualEndFrame() {
  auto inputBufferLength = static_cast<double>(buffer_->bus_->getSize());
  auto loopEndFrame = loopEnd_ * context_->getSampleRate();

  return loop_ && loopEndFrame > 0 && loopStart_ < loopEnd_
      ? std::min(loopEndFrame, inputBufferLength)
      : inputBufferLength;
}

} // namespace audioapi
