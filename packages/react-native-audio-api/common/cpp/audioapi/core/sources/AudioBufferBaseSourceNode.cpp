#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>

namespace audioapi {
AudioBufferBaseSourceNode::AudioBufferBaseSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const BaseAudioBufferSourceOptions &options)
    : AudioScheduledSourceNode(context, options),
      pitchCorrection_(options.pitchCorrection),
      playbackRateBuffer_(
          std::make_shared<AudioBuffer>(
              RENDER_QUANTUM_SIZE * 3,
              channelCount_,
              context->getSampleRate())),
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
      vReadIndex_(0.0),
      onPositionChangedInterval_(
          static_cast<int>(context->getSampleRate() * ON_POSITION_CHANGED_INTERVAL_SECONDS)) {}
void AudioBufferBaseSourceNode::initStretch(
    const std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> &stretch) {
  stretch_ = stretch;
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
  onPositionChangedInterval_ = static_cast<int>(
      getContextSampleRate() * static_cast<float>(interval) *
      1e-3f); //NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
}

int AudioBufferBaseSourceNode::getOnPositionChangedInterval() const {
  return onPositionChangedInterval_;
}

void AudioBufferBaseSourceNode::unregisterOnPositionChangedCallback(uint64_t callbackId) {
  audioEventHandlerRegistry_->unregisterHandler(AudioEvent::POSITION_CHANGED, callbackId);
}

void AudioBufferBaseSourceNode::sendOnPositionChangedEvent() {
  if (onPositionChangedCallbackId_ != 0 && onPositionChangedTime_ > onPositionChangedInterval_) {
    std::unordered_map<std::string, EventValue> body = {{"value", getCurrentPosition()}};

    audioEventHandlerRegistry_->invokeHandlerWithEventBody(
        AudioEvent::POSITION_CHANGED, onPositionChangedCallbackId_, body);

    onPositionChangedTime_ = 0;
  }

  onPositionChangedTime_ += RENDER_QUANTUM_SIZE;
}

void AudioBufferBaseSourceNode::processWithPitchCorrection(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
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

  auto framesNeededToStretch = static_cast<int>(playbackRate * static_cast<float>(framesToProcess));

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

  processWithoutInterpolation(playbackRateBuffer_, startOffset, offsetLength, playbackRate);

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
    const std::shared_ptr<AudioBuffer> &processingBuffer,
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
    processWithoutInterpolation(processingBuffer, startOffset, offsetLength, computedPlaybackRate);
  } else {
    processWithInterpolation(processingBuffer, startOffset, offsetLength, computedPlaybackRate);
  }

  sendOnPositionChangedEvent();
}

float AudioBufferBaseSourceNode::getComputedPlaybackRateValue(int framesToProcess, double time) {
  auto playbackRate = playbackRateParam_->processKRateParam(framesToProcess, time);
  auto detune = std::pow(
      2.0f, //NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
      detuneParam_->processKRateParam(framesToProcess, time) / static_cast<float>(OCTAVE_RANGE));

  return playbackRate * detune;
}

} // namespace audioapi
