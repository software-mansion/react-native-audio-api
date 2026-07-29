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
#include <utility>

#include <iostream>

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
  wsolaPrimeDebugPending_ = true;
}

void AudioBufferBaseSourceNode::primeWsolaInput() {
  if (!pitchCorrection_ || playbackRateBuffer_ == nullptr || isEmpty()) {
    return;
  }

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    return;
  }

  const float rate = std::fabs(
      playbackRateParam_->processKRateParam(RENDER_QUANTUM_SIZE, context->getCurrentTime()));
  // WSOLA path is only used when |rate| != 1; skip priming otherwise.
  if (rate == 0.0f || rate == 1.0f) {
    return;
  }

  wsolaStretcher_.reset();

  const size_t framesNeeded =
      std::max(wsolaStretcher_.getRequiredInputFrames(), wsolaStretcher_.getMinInputFramesToRun());
  if (framesNeeded == 0) {
    return;
  }

  const size_t inputFrames = std::min(framesNeeded, playbackRateBuffer_->getSize());
  playbackRateBuffer_->zero();

  // 1:1 forward copy into WSOLA's analysis queue; advances vReadIndex_ so the
  // cursor stays aligned with what was fed before the first output quantum.
  runBufferProcessor(playbackRateBuffer_, 0, inputFrames, 1.0f, false);
  wsolaStretcher_.feedInput(*playbackRateBuffer_, inputFrames);

  if (wsolaPrimeDebugPending_) {
    wsolaPrimeDebugPending_ = false;
    std::cout << "[AudioBufferBaseSourceNode] [WSOLA prime at start]"
              << " buffered=" << wsolaStretcher_.getBufferedInputFrames()
              << " required=" << framesNeeded << std::endl
              << std::endl;
  }
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

  // apply pitch correction only if the playback rate is not 1.0
  if (pitchCorrection_ &&
      getComputedPlaybackRateValue(framesToProcess, context_.lock()->getCurrentTime()) != 1.0f) {
    processWithPitchCorrection(audioBuffer_, framesToProcess);
  } else {
    processWithoutPitchCorrection(audioBuffer_, framesToProcess);
  }

  handleStopScheduled();
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
  const double time = context->getCurrentTime();
  const auto rate = std::clamp(
      playbackRateParam_->processKRateParam(framesToProcess, time),
      -WsolaTimeStretcher::MAX_PLAYBACK_RATE,
      WsolaTimeStretcher::MAX_PLAYBACK_RATE);

  // Prefill happens only in primeWsolaInput() at start(); here feed one quantum's
  // worth of input for the current rate.
  const float absRate = std::fabs(rate);
  const size_t requestedInputFrames = std::max(
      size_t{1}, static_cast<size_t>(std::ceil(absRate * static_cast<float>(framesToProcess))));
  const size_t inputFrames = std::min(requestedInputFrames, playbackRateBuffer_->getSize());
  const int framesNeededToStretch = static_cast<int>(inputFrames);

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

  const auto detune = detuneParam_->processKRateParam(framesToProcess, time) / 100.0f;
  const float pitchFactor = std::pow(
      2.0f, // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
      detune / static_cast<float>(SEMITONES_PER_OCTAVE));

  // Non-interpolated path uses rate only for direction; abs keeps forward copies 1:1.
  runBufferProcessor(playbackRateBuffer_, startOffset, offsetLength, absRate, false);

  wsolaStretcher_.process(
      *playbackRateBuffer_,
      inputFrames,
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

  if (isPlaying()) {
    positionChanged_.advance(RENDER_QUANTUM_SIZE, getCurrentPosition());
  }
}

float AudioBufferBaseSourceNode::getComputedPlaybackRateValue(int framesToProcess, double time) {
  auto playbackRate = std::clamp(
      playbackRateParam_->processKRateParam(framesToProcess, time),
      -WsolaTimeStretcher::MAX_PLAYBACK_RATE,
      WsolaTimeStretcher::MAX_PLAYBACK_RATE);
  auto detune = std::pow(
      2.0f, //NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
      detuneParam_->processKRateParam(framesToProcess, time) / static_cast<float>(OCTAVE_RANGE));

  return playbackRate * detune;
}

} // namespace audioapi
