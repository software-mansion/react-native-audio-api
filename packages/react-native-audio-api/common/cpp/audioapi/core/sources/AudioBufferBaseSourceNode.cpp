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

void AudioBufferBaseSourceNode::primeWsolaInput() {
  if (!pitchCorrection_ || playbackRateBuffer_ == nullptr || isEmpty()) {
    return;
  }

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    return;
  }

  const float rate = std::fabs(playbackRateParam_->processKRateParam(context->getCurrentTime()));
  // WSOLA path is only used when |rate| != 1; skip priming otherwise.
  if (rate == 0.0f || rate == 1.0f) {
    return;
  }

  wsolaDrainPending_ = false;
  wsolaStretcher_.reset();
  wsolaExpectedOutputFrames_ = 0.0;
  wsolaEmittedOutputFrames_ = 0.0;

  const size_t framesNeeded =
      std::max(wsolaStretcher_.getRequiredInputFrames(), wsolaStretcher_.getMinInputFramesToRun());
  if (framesNeeded == 0) {
    return;
  }

  const size_t inputFrames = std::min(framesNeeded, playbackRateBuffer_->getSize());
  playbackRateBuffer_->zero();

  // 1:1 forward copy into WSOLA's analysis queue; advances vReadIndex_ so the
  // cursor stays aligned with what was fed before the first output quantum.
  // Preserve playbackState_: runBufferProcessor can mark STOP_SCHEDULED if the
  // prime consumes through EOF, which would finish the node before first output.
  const auto savedState = playbackState_;
  runBufferProcessor(playbackRateBuffer_, 0, inputFrames, 1.0f, false);
  playbackState_ = savedState;
  wsolaStretcher_.feedInput(*playbackRateBuffer_, inputFrames);

  // Primed PCM occupies inputFrames / rate of output time once stretched.
  wsolaExpectedOutputFrames_ = static_cast<double>(inputFrames) / static_cast<double>(rate);
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
  // Drain before isEmpty(): queue sources pop their last buffer before the OLA
  // tail is flushed, so buffers_ is empty while wsolaDrainPending_ is still armed.
  if (wsolaDrainPending_) {
    processWsolaDrain(audioBuffer_, framesToProcess);
    handleStopScheduled();
    return;
  }

  if (isEmpty()) {
    audioBuffer_->zero();
    // Queue endOfStream() can leave STOP_SCHEDULED with an already-empty queue
    // (mark after underrun). Natural stop (no explicit stopTime_) still needs
    // to flush leftover WSOLA output when pitch correction is active.
    if (isStopScheduled() && stopTime_ < 0.0 && pitchCorrection_) {
      std::shared_ptr<BaseAudioContext> context = context_.lock();
      if (context != nullptr) {
        const float absRate =
            std::fabs(playbackRateParam_->processKRateParam(context->getCurrentTime()));
        playbackState_ = PlaybackState::PLAYING;
        wsolaDrainPending_ = true;
        wsolaEofDrainRate_ = absRate > 0.0f ? absRate : 1.0f;
        processWsolaDrain(audioBuffer_, framesToProcess);
      }
    }
    handleStopScheduled();
    return;
  }

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    audioBuffer_->zero();
    return;
  }

  // Param caches include time in their key, so every read in this quantum must
  // use the same value.
  const double time = context->getCurrentTime();

  // TODO: check if we want to use interpolation path for 1.0 playback speed when pitch correction is disabled
  if (pitchCorrection_) {
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
  // WSOLA receives playback speed and pitch as separate inputs, unlike regular
  // playback, which uses their spec-defined product. Its intermediate buffer
  // is sized only for |rate| <= MAX_PLAYBACK_RATE.
  const auto rate = std::clamp(
      playbackRateParam_->processKRateParam(time),
      -WsolaTimeStretcher::MAX_PLAYBACK_RATE,
      WsolaTimeStretcher::MAX_PLAYBACK_RATE);
  const float absRate = std::fabs(rate);

  // Output-quantum domain: startOffset/offsetLength describe the audible span of
  // this quantum, so pre-start and post-stop zeroing lands in the real output.
  updatePlaybackInfo(
      processingBuffer,
      framesToProcess,
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

  // Prefill happens only in primeWsolaInput() at start(); here feed input sized to
  // the audible span. Feed real PCM only — leading start-quantum silence must not
  // enter the analysis queue, or it surfaces as a dropout at the moment the primed
  // input is consumed (mid-quantum starts would inject up to a quantum of zeros
  // between primed and streamed input).
  const size_t requestedInputFrames = std::max(
      size_t{1}, static_cast<size_t>(std::ceil(absRate * static_cast<float>(offsetLength))));
  const size_t inputFrames = std::min(requestedInputFrames, playbackRateBuffer_->getSize());

  playbackRateBuffer_->zero();

  // Non-interpolated path uses rate only for direction; abs keeps forward copies 1:1.
  // updatePlaybackInfo may already have armed STOP_SCHEDULED from an explicit stop(when).
  // runBufferProcessor arms it only on natural PCM EOF — drain that case only.
  const bool stopFromExplicitSchedule = isStopScheduled();
  const double readIndexBefore = vReadIndex_;
  runBufferProcessor(playbackRateBuffer_, 0, inputFrames, absRate, false);
  // Cursor delta = PCM actually consumed (EOF quantum may feed less than requested).
  wsolaExpectedOutputFrames_ +=
      std::fabs(vReadIndex_ - readIndexBefore) / static_cast<double>(absRate);

  wsolaStretcher_.process(
      *playbackRateBuffer_, inputFrames, *processingBuffer, offsetLength, rate, pitchFactor);
  wsolaEmittedOutputFrames_ += static_cast<double>(offsetLength);

  if (startOffset > 0) {
    // Mid-quantum start: WSOLA rendered [0, offsetLength); shift into the
    // scheduled position so sound does not begin before startTime_.
    for (size_t i = 0; i < processingBuffer->getNumberOfChannels(); ++i) {
      auto *channel = processingBuffer->getChannel(i);
      for (size_t j = offsetLength; j > 0; --j) {
        (*channel)[j - 1 + startOffset] = (*channel)[j - 1];
      }
    }
    processingBuffer->zero(0, startOffset);
  }

  // Natural EOF: keep playing while WSOLA flushes its OLA tail. Explicit stop(when)
  // must cut immediately — draining would overlap the next scheduled source.
  if (isStopScheduled() && !stopFromExplicitSchedule) {
    playbackState_ = PlaybackState::PLAYING;
    wsolaDrainPending_ = true;
    wsolaEofDrainRate_ = absRate > 0.0f ? absRate : 1.0f;
  }

  if (isPlaying()) {
    positionChanged_.advance(RENDER_QUANTUM_SIZE, getCurrentPosition());
  }
}

void AudioBufferBaseSourceNode::processWsolaDrain(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  const auto frames = static_cast<size_t>(framesToProcess);
  processingBuffer->zero();

  const double remaining = wsolaExpectedOutputFrames_ - wsolaEmittedOutputFrames_;
  if (remaining <= 0.0) {
    wsolaStretcher_.finalizeDrainTailDump(wsolaEofDrainRate_);
    wsolaDrainPending_ = false;
    playbackState_ = PlaybackState::STOP_SCHEDULED;
    return;
  }

  // Cap this quantum to the remaining content budget so silence-padded OLA hops
  // cannot extend past duration/rate and overlap the next scheduled source.
  const auto framesToDrain = std::min(frames, static_cast<size_t>(std::ceil(remaining)));
  const size_t drained =
      wsolaStretcher_.drainOutput(*processingBuffer, framesToDrain, wsolaEofDrainRate_);
  wsolaEmittedOutputFrames_ += static_cast<double>(drained);

  const bool contentDelivered = wsolaEmittedOutputFrames_ >= wsolaExpectedOutputFrames_;
  if (drained >= frames && !contentDelivered) {
    return;
  }

  wsolaStretcher_.finalizeDrainTailDump(wsolaEofDrainRate_);
  wsolaDrainPending_ = false;
  playbackState_ = PlaybackState::STOP_SCHEDULED;
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
