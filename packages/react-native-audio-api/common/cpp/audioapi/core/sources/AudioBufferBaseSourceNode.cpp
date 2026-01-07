#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace audioapi {
AudioBufferBaseSourceNode::AudioBufferBaseSourceNode(
    std::shared_ptr<BaseAudioContext> context,
    bool pitchCorrection)
    : AudioScheduledSourceNode(context),
      pitchCorrection_(pitchCorrection),
      stretch_(std::make_shared<signalsmith::stretch::SignalsmithStretch<float>>()),
      playbackRateBus_(
          std::make_shared<AudioBus>(
              RENDER_QUANTUM_SIZE * 3,
              channelCount_,
              context->getSampleRate())),
      detuneParam_(
          std::make_shared<AudioParam>(
              0.0,
              MOST_NEGATIVE_SINGLE_FLOAT,
              MOST_POSITIVE_SINGLE_FLOAT,
              context)),
      playbackRateParam_(
          std::make_shared<AudioParam>(
              1.0,
              MOST_NEGATIVE_SINGLE_FLOAT,
              MOST_POSITIVE_SINGLE_FLOAT,
              context)),
      vReadIndex_(0.0),
      onPositionChangedInterval_(static_cast<int>(context->getSampleRate() * 0.1f)) {}

std::shared_ptr<AudioParam> AudioBufferBaseSourceNode::getDetuneParam() const {
  return detuneParam_;
}

std::shared_ptr<AudioParam> AudioBufferBaseSourceNode::getPlaybackRateParam() const {
  return playbackRateParam_;
}

void AudioBufferBaseSourceNode::setOnPositionChangedCallbackId(uint64_t callbackId) {
  auto oldCallbackId = onPositionChangedCallbackId_.exchange(callbackId, std::memory_order_acq_rel);

  if (oldCallbackId != 0) {
    audioEventHandlerRegistry_->unregisterHandler("positionChanged", oldCallbackId);
  }
}

void AudioBufferBaseSourceNode::setOnPositionChangedInterval(int interval) {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    onPositionChangedInterval_ =
        static_cast<int>(context->getSampleRate() * static_cast<float>(interval) / 1000);
  }
}

int AudioBufferBaseSourceNode::getOnPositionChangedInterval() const {
  return onPositionChangedInterval_;
}

std::mutex &AudioBufferBaseSourceNode::getBufferLock() {
  return bufferLock_;
}

double AudioBufferBaseSourceNode::getInputLatency() const {
  if (pitchCorrection_) {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      return static_cast<double>(stretch_->inputLatency()) / context->getSampleRate();
    } else {
      return 0;
    }
  }
  return 0;
}

double AudioBufferBaseSourceNode::getOutputLatency() const {
  if (pitchCorrection_) {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      return static_cast<double>(stretch_->outputLatency()) / context->getSampleRate();
    } else {
      return 0;
    }
  }
  return 0;
}

void AudioBufferBaseSourceNode::sendOnPositionChangedEvent() {
  auto onPositionChangedCallbackId = onPositionChangedCallbackId_.load(std::memory_order_acquire);

  if (onPositionChangedCallbackId != 0 && onPositionChangedTime_ > onPositionChangedInterval_) {
    std::unordered_map<std::string, EventValue> body = {{"value", getCurrentPosition()}};

    audioEventHandlerRegistry_->invokeHandlerWithEventBody(
        "positionChanged", onPositionChangedCallbackId, body);

    onPositionChangedTime_ = 0;
  }

  onPositionChangedTime_ += RENDER_QUANTUM_SIZE;
}

void AudioBufferBaseSourceNode::processWithPitchCorrection(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBus->zero();
    return;
  }
  auto time = context->getCurrentTime();
  auto playbackRate =
      std::clamp(playbackRateParam_->processKRateParam(framesToProcess, time), 0.0f, 3.0f);
  auto detune =
      std::clamp(detuneParam_->processKRateParam(framesToProcess, time) / 100.0f, -12.0f, 12.0f);

  playbackRateBus_->zero();

  auto framesNeededToStretch = static_cast<int>(playbackRate * static_cast<float>(framesToProcess));

  updatePlaybackInfo(
      playbackRateBus_,
      framesNeededToStretch,
      startOffset,
      offsetLength,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (playbackRate == 0.0f || (!isPlaying() && !isStopScheduled())) {
    processingBus->zero();
    return;
  }

  processWithoutInterpolation(playbackRateBus_, startOffset, offsetLength, playbackRate);

  stretch_->process(
      playbackRateBus_.get()[0], framesNeededToStretch, processingBus.get()[0], framesToProcess);

  if (detune != 0.0f) {
    stretch_->setTransposeSemitones(detune);
  }

  sendOnPositionChangedEvent();
}

void AudioBufferBaseSourceNode::processWithoutPitchCorrection(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBus->zero();
    return;
  }
  auto computedPlaybackRate =
      getComputedPlaybackRateValue(framesToProcess, context->getCurrentTime());
  updatePlaybackInfo(processingBus, framesToProcess, startOffset, offsetLength, context->getSampleRate(), context->getCurrentSampleFrame());

  if (computedPlaybackRate == 0.0f || (!isPlaying() && !isStopScheduled())) {
    processingBus->zero();
    return;
  }

  if (std::fabs(computedPlaybackRate) == 1.0) {
    processWithoutInterpolation(processingBus, startOffset, offsetLength, computedPlaybackRate);
  } else {
    processWithInterpolation(processingBus, startOffset, offsetLength, computedPlaybackRate);
  }

  sendOnPositionChangedEvent();
}

float AudioBufferBaseSourceNode::getComputedPlaybackRateValue(int framesToProcess, double time) {
  auto playbackRate = playbackRateParam_->processKRateParam(framesToProcess, time);
  auto detune = std::pow(2.0f, detuneParam_->processKRateParam(framesToProcess, time) / 1200.0f);

  return playbackRate * detune;
}

} // namespace audioapi
