#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <algorithm>
#include <memory>

namespace audioapi {
AudioBufferBaseSourceNode::AudioBufferBaseSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const BaseAudioBufferSourceOptions &options)
    : AudioScheduledSourceNode(context, options),
      vReadIndex_(0.0),
      pitchCorrection_(options.pitchCorrection),
      detuneParam_(
          std::make_shared<AudioParam>(
              options.detune,
              MOST_NEGATIVE_SINGLE_FLOAT,
              MOST_POSITIVE_SINGLE_FLOAT,
              context)),
      playbackRateParam_(
          std::make_shared<AudioParam>(
              options.playbackRate,
              MOST_NEGATIVE_SINGLE_FLOAT,
              MOST_POSITIVE_SINGLE_FLOAT,
              context)),
      onPositionChangedIntervalInFrames_(static_cast<int>(context->getSampleRate())) {
  setOnPositionChangedInterval(options.onPositionChangedInterval);
}

void AudioBufferBaseSourceNode::initStretch(
    const std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> &stretch,
    const std::shared_ptr<DSPAudioBuffer> &playbackRateBuffer) {
  stretch_ = stretch;
  playbackRateBuffer_ = playbackRateBuffer;
}

std::shared_ptr<AudioParam> AudioBufferBaseSourceNode::getDetuneParam() const {
  return detuneParam_;
}

std::shared_ptr<AudioParam> AudioBufferBaseSourceNode::getPlaybackRateParam() const {
  return playbackRateParam_;
}

void AudioBufferBaseSourceNode::setOnPositionChangedCallbackId(uint64_t callbackId) {
  onPositionChangedCallbackId_ = callbackId;
}

void AudioBufferBaseSourceNode::setOnPositionChangedInterval(int interval) {
  onPositionChangedIntervalInFrames_ = static_cast<int>(
      //NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
      getContextSampleRate() * static_cast<float>(interval) / 1000);
}

void AudioBufferBaseSourceNode::unregisterOnPositionChangedCallback(uint64_t callbackId) {
  audioEventHandlerRegistry_->unregisterHandler(AudioEvent::POSITION_CHANGED, callbackId);
}

std::shared_ptr<DSPAudioBuffer> AudioBufferBaseSourceNode::processNode(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (isEmpty()) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (!pitchCorrection_) {
    processWithoutPitchCorrection(processingBuffer, framesToProcess);
  } else {
    processWithPitchCorrection(processingBuffer, framesToProcess);
  }

  handleStopScheduled();

  return processingBuffer;
}

void AudioBufferBaseSourceNode::sendOnPositionChangedEvent() {
  if (onPositionChangedCallbackId_ != 0 &&
      onPositionChangedTimeInFrames_ > onPositionChangedIntervalInFrames_) {
    audioEventHandlerRegistry_->dispatchEvent(
        AudioEvent::POSITION_CHANGED,
        onPositionChangedCallbackId_,
        DoubleValuePayload{.value = getCurrentPosition()});

    onPositionChangedTimeInFrames_ = 0;
  }

  onPositionChangedTimeInFrames_ += RENDER_QUANTUM_SIZE;
}

void AudioBufferBaseSourceNode::processWithPitchCorrection(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr || playbackRateBuffer_ == nullptr) {
    processingBuffer->zero();
    return;
  }
  auto time = context->getCurrentTime();
  auto playbackRate = std::clamp(
      playbackRateParam_->processKRateParam(framesToProcess, time),
      MIN_PLAYBACK_RATE,
      MAX_PLAYBACK_RATE);
  auto detune = std::clamp(
      detuneParam_->processKRateParam(framesToProcess, time) / 100.0f,
      static_cast<float>(-SEMITONES_PER_OCTAVE),
      static_cast<float>(SEMITONES_PER_OCTAVE));

  playbackRateBuffer_->zero();

  auto framesNeededToStretch =
      std::abs(static_cast<int>(playbackRate * static_cast<float>(framesToProcess)));

  updatePlaybackInfo(
      playbackRateBuffer_,
      framesNeededToStretch,
      startOffset,
      offsetLength,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (playbackRate == 0.0f || (!isPlaying() && !isStopScheduled()) || stretch_ == nullptr) {
    processingBuffer->zero();
    return;
  }

  runBufferProcessor(playbackRateBuffer_, startOffset, offsetLength, playbackRate, false);

  stretch_->process(
      playbackRateBuffer_.get()[0],
      framesNeededToStretch,
      processingBuffer.get()[0],
      framesToProcess);

  if (detune != 0.0f) {
    stretch_->setTransposeSemitones(detune);
  }

  sendOnPositionChangedEvent();
}

void AudioBufferBaseSourceNode::processWithoutPitchCorrection(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBuffer->zero();
    return;
  }

  auto computedPlaybackRate =
      getComputedPlaybackRateValue(framesToProcess, context->getCurrentTime());

  updatePlaybackInfo(
      processingBuffer,
      framesToProcess,
      startOffset,
      offsetLength,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (computedPlaybackRate == 0.0f || (!isPlaying() && !isStopScheduled())) {
    processingBuffer->zero();
    return;
  }

  if (std::fabs(computedPlaybackRate) == 1.0) {
    runBufferProcessor(processingBuffer, startOffset, offsetLength, computedPlaybackRate, false);
  } else {
    runBufferProcessor(processingBuffer, startOffset, offsetLength, computedPlaybackRate, true);
  }

  sendOnPositionChangedEvent();
}

float AudioBufferBaseSourceNode::getComputedPlaybackRateValue(int framesToProcess, double time) {
  auto playbackRate = std::clamp(
      playbackRateParam_->processKRateParam(framesToProcess, time),
      MIN_PLAYBACK_RATE,
      MAX_PLAYBACK_RATE);
  auto detune = std::pow(
      2.0f, //NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
      detuneParam_->processKRateParam(framesToProcess, time) / static_cast<float>(OCTAVE_RANGE));

  return playbackRate * detune;
}

} // namespace audioapi
