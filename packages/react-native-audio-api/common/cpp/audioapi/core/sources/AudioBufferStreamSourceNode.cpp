#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/Constants.h>
#include <audioapi/core/sources/AudioBufferStreamSourceNode.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

AudioBufferStreamSourceNode::AudioBufferStreamSourceNode(
    BaseAudioContext *context)
    : AudioScheduledSourceNode(context), vReadIndex_(0.0) {
  buffers_ = {};

  detuneParam_ = std::make_shared<AudioParam>(
      0.0, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);
  playbackRateParam_ = std::make_shared<AudioParam>(
      1.0, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);

  playbackRateBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE * 3, channelCount_, context_->getSampleRate());

  stretch_ =
      std::make_shared<signalsmith::stretch::SignalsmithStretch<float>>();
  stretch_->presetDefault(channelCount_, context_->getSampleRate(), true);

  isInitialized_ = true;
}

AudioBufferStreamSourceNode::~AudioBufferStreamSourceNode() {
  Locker locker(getBufferLock());

  buffers_ = {};
}

std::shared_ptr<AudioParam> AudioBufferStreamSourceNode::getDetuneParam()
    const {
  return detuneParam_;
}

std::shared_ptr<AudioParam> AudioBufferStreamSourceNode::getPlaybackRateParam()
    const {
  return playbackRateParam_;
}

void AudioBufferStreamSourceNode::start(double when, double offset) {
  AudioScheduledSourceNode::start(when);

  vReadIndex_ = static_cast<double>(context_->getSampleRate() * offset);
}

void AudioBufferStreamSourceNode::enqueueBuffer(
    const std::shared_ptr<AudioBuffer> &buffer,
    bool isLastBuffer) {
  auto locker = Locker(getBufferLock());
  buffers_.push(buffer);

  isLastBuffer_ = isLastBuffer;
}

void AudioBufferStreamSourceNode::disable() {
  AudioScheduledSourceNode::disable();
  buffers_ = {};
}

std::mutex &AudioBufferStreamSourceNode::getBufferLock() {
  return bufferLock_;
}

void AudioBufferStreamSourceNode::processNode(
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

double AudioBufferStreamSourceNode::getStopTime() const {
  return dsp::sampleFrameToTime(
      static_cast<int>(vReadIndex_), context_->getSampleRate());
}

/**
 * Helper functions
 */

void AudioBufferStreamSourceNode::processWithPitchCorrection(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  auto time = context_->getCurrentTime();
  auto playbackRate = std::clamp(
      playbackRateParam_->processKRateParam(framesToProcess, time), 0.0f, 3.0f);
  auto detune = std::clamp(
      detuneParam_->processKRateParam(framesToProcess, time) / 100.0f,
      -12.0f,
      12.0f);

  playbackRateBus_->zero();

  updatePlaybackInfo(
      processingBus, framesToProcess, startOffset, offsetLength);

  if (playbackRate == 0.0f || (!isPlaying() && !isStopScheduled())) {
    processingBus->zero();
    return;
  }

    auto framesNeededToStretch =
            static_cast<int>(playbackRate * static_cast<float>(framesToProcess));
  auto stretchedStartOffset = static_cast<size_t>(static_cast<float>(startOffset) * playbackRate);
    auto stretchedOffsetLength = static_cast<size_t>(static_cast<float>(offsetLength) * playbackRate);

  processWithoutInterpolation(playbackRateBus_, stretchedStartOffset, stretchedOffsetLength);

  stretch_->process(
      playbackRateBus_.get()[0],
      framesNeededToStretch,
      processingBus.get()[0],
      framesToProcess);

  if (detune != 0.0f) {
    stretch_->setTransposeSemitones(detune);
  }
}

void AudioBufferStreamSourceNode::processWithoutInterpolation(
    const std::shared_ptr<AudioBus> &processingBus,
    size_t startOffset,
    size_t offsetLength) {
  auto readIndex = static_cast<size_t>(vReadIndex_);
  size_t writeIndex = startOffset;

  auto buffer = buffers_.front();
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
      buffers_.pop();

      if (buffers_.empty()) {
        processingBus->zero(writeIndex, framesLeft);
        readIndex = 0;

        if (isLastBuffer_) {
            playbackState_ = PlaybackState::STOP_SCHEDULED;
        }
        break;
      } else {
        buffer = buffers_.front();
        readIndex = 0;
      }
    }
  }

  // update reading index for next render quantum
  vReadIndex_ = static_cast<double>(readIndex);
}

} // namespace audioapi
