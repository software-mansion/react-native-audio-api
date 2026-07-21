#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <audioapi/dsp/WsolaTimeStretcher.h>
#include <algorithm>
#include <cmath>
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
      computedPlaybackRateParam_(
          std::make_shared<CompositeAudioParam<combineComputedPlaybackRate>>(
              MOST_NEGATIVE_SINGLE_FLOAT,
              MOST_POSITIVE_SINGLE_FLOAT,
              context,
              playbackRateParam_,
              detuneParam_)),
      positionChanged_(
          context->getAudioEventHandlerRegistry(),
          static_cast<int>(context->getSampleRate())) {
  setOnPositionChangedInterval(options.onPositionChangedInterval);
}

void AudioBufferBaseSourceNode::initStretch(
    size_t channelCount,
    float sampleRate,
    const std::shared_ptr<DSPAudioBuffer> &playbackRateBuffer) {
  wsolaStretcher_.configure(channelCount, sampleRate);
  playbackRateBuffer_ = playbackRateBuffer;
}

std::shared_ptr<AudioParam> AudioBufferBaseSourceNode::getDetuneParam() const {
  return detuneParam_;
}

std::shared_ptr<AudioParam> AudioBufferBaseSourceNode::getPlaybackRateParam() const {
  return playbackRateParam_;
}

void AudioBufferBaseSourceNode::setOnPositionChangedInterval(int interval) {
  positionChanged_.setIntervalMs(interval, getContextSampleRate());
}

void AudioBufferBaseSourceNode::assignOnPositionChangedCallbackId(uint64_t callbackId) {
  positionChanged_.assignCallbackId(callbackId);
}

void AudioBufferBaseSourceNode::processNode(int framesToProcess) {
  if (isEmpty()) {
    audioBuffer_->zero();
    return;
  }

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    audioBuffer_->zero();
    return;
  }

  // Read the context clock once per render quantum and thread it through every
  // param process call so the idempotent cache keys line up (see plan: Time invariant).
  const double time = context->getCurrentTime();

  // apply pitch correction only if the playback rate is not 1.0
  if (pitchCorrection_ && computedPlaybackRateParam_->processKRateParam(time) != 1.0f) {
    processWithPitchCorrection(audioBuffer_, framesToProcess, time);
  } else {
    processWithoutPitchCorrection(audioBuffer_, framesToProcess, time);
  }

  handleStopScheduled();
}

void AudioBufferBaseSourceNode::processWithPitchCorrection(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess,
    double time) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr || playbackRateBuffer_ == nullptr) {
    processingBuffer->zero();
    return;
  }
  // Read the constituents separately (cache hits — the composite already
  // processed both children for this quantum). Clamp to the WSOLA-supported
  // range: stretch buffers are sized for |rate| ≤ MAX_PLAYBACK_RATE.
  const auto rate = std::clamp(
      playbackRateParam_->processKRateParam(time),
      -WsolaTimeStretcher::MAX_PLAYBACK_RATE,
      WsolaTimeStretcher::MAX_PLAYBACK_RATE);

  const int framesNeededToStretch =
      std::max(1, static_cast<int>(std::ceil(rate * framesToProcess)));

  playbackRateBuffer_->zero();

  updatePlaybackInfo(
      playbackRateBuffer_,
      framesNeededToStretch,
      startOffset,
      offsetLength,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (rate == 0.0f || (!isPlaying() && !isStopScheduled())) {
    processingBuffer->zero();
    return;
  }

  const auto detune = detuneParam_->processKRateParam(time) / 100.0f;
  const float pitchFactor = std::pow(
      2.0f, // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
      detune / static_cast<float>(SEMITONES_PER_OCTAVE));

  const float bufferPlaybackRate = rate >= 0.0f ? rate : -rate;
  runBufferProcessor(playbackRateBuffer_, startOffset, offsetLength, bufferPlaybackRate, false);

  wsolaStretcher_.process(
      *playbackRateBuffer_,
      static_cast<size_t>(framesNeededToStretch),
      *processingBuffer,
      static_cast<size_t>(framesToProcess),
      rate,
      pitchFactor);

  if (isPlaying()) {
    positionChanged_.advance(RENDER_QUANTUM_SIZE, getCurrentPosition());
  }
}

void AudioBufferBaseSourceNode::processWithoutPitchCorrection(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess,
    double time) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBuffer->zero();
    return;
  }

  auto computedPlaybackRate = computedPlaybackRateParam_->processKRateParam(time);

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

  if (isPlaying()) {
    positionChanged_.advance(RENDER_QUANTUM_SIZE, getCurrentPosition());
  }
}

} // namespace audioapi
