#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>

#if !RN_AUDIO_API_TEST
#include <audioapi/core/AudioContext.h>
#endif // RN_AUDIO_API_TEST

#include <algorithm>
#include <limits>
#include <memory>

namespace audioapi {

AudioScheduledSourceNode::AudioScheduledSourceNode(
    const std::shared_ptr<BaseAudioContext>& context,
    const AudioScheduledSourceNodeOptions &options)
    : AudioNode(context, options),
      startTime_(-1.0),
      stopTime_(-1.0),
      playbackState_(PlaybackState::UNSCHEDULED),
      audioEventHandlerRegistry_(context->getAudioEventHandlerRegistry()) {}

void AudioScheduledSourceNode::start(double when) {
#if !RN_AUDIO_API_TEST
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    if (auto audioContext = dynamic_cast<AudioContext *>(context.get())) {
      audioContext->start();
    }
  }
#endif // RN_AUDIO_API_TEST

  playbackState_ = PlaybackState::SCHEDULED;
  startTime_ = when;
}

void AudioScheduledSourceNode::stop(double when) {
  stopTime_ = when;
}

bool AudioScheduledSourceNode::isUnscheduled() {
  return playbackState_ == PlaybackState::UNSCHEDULED;
}

bool AudioScheduledSourceNode::isScheduled() {
  return playbackState_ == PlaybackState::SCHEDULED;
}

bool AudioScheduledSourceNode::isPlaying() {
  return playbackState_ == PlaybackState::PLAYING;
}

bool AudioScheduledSourceNode::isFinished() {
  return playbackState_ == PlaybackState::FINISHED;
}

bool AudioScheduledSourceNode::isStopScheduled() {
  return playbackState_ == PlaybackState::STOP_SCHEDULED;
}

void AudioScheduledSourceNode::setOnEndedCallbackId(const uint64_t callbackId) {
  auto oldCallbackId = onEndedCallbackId_.exchange(callbackId, std::memory_order_acq_rel);

  if (oldCallbackId != 0) {
    audioEventHandlerRegistry_->unregisterHandler(AudioEvent::ENDED, oldCallbackId);
  }
}

void AudioScheduledSourceNode::updatePlaybackInfo(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess,
    size_t &startOffset,
    size_t &nonSilentFramesToProcess,
    float sampleRate,
    size_t currentSampleFrame) {
  if (!isInitialized_) {
    startOffset = 0;
    nonSilentFramesToProcess = 0;
    return;
  }

  auto firstFrame = currentSampleFrame;
  size_t lastFrame = firstFrame + framesToProcess - 1;

  size_t startFrame = std::max(dsp::timeToSampleFrame(startTime_, sampleRate), firstFrame);
  size_t stopFrame = stopTime_ == -1.0 ? std::numeric_limits<size_t>::max()
                                       : dsp::timeToSampleFrame(stopTime_, sampleRate);
  if (isFinished()) {
    startOffset = 0;
    nonSilentFramesToProcess = 0;
    return;
  }

  if (isScheduled()) {
    // not yet playing
    if (startFrame > lastFrame) {
      startOffset = 0;
      nonSilentFramesToProcess = 0;
      return;
    }

    // start playing
    // zero first frames before starting frame
    playbackState_ = PlaybackState::PLAYING;
    startOffset = std::max(startFrame, firstFrame) - firstFrame > 0
        ? std::max(startFrame, firstFrame) - firstFrame
        : 0;
    nonSilentFramesToProcess =
        std::max(std::min(lastFrame, stopFrame) + 1, startFrame) - startFrame;

    assert(startOffset <= framesToProcess);
    assert(nonSilentFramesToProcess <= framesToProcess);

    // stop will happen in the same render quantum
    if (stopFrame <= lastFrame && stopFrame >= firstFrame) {
      playbackState_ = PlaybackState::STOP_SCHEDULED;
      processingBuffer->zero(stopFrame - firstFrame, lastFrame - stopFrame);
    }

    processingBuffer->zero(0, startOffset);
    return;
  }

  // the node is playing

  // stop will happen in this render quantum
  // zero remaining frames after stop frame
  if (stopFrame <= lastFrame && stopFrame >= firstFrame) {
    playbackState_ = PlaybackState::STOP_SCHEDULED;
    startOffset = 0;
    nonSilentFramesToProcess = stopFrame - firstFrame;

    assert(startOffset <= framesToProcess);
    assert(nonSilentFramesToProcess <= framesToProcess);

    processingBuffer->zero(stopFrame - firstFrame, lastFrame - stopFrame);
    return;
  }

  // mark as finished in first silent render quantum
  if (stopFrame < firstFrame) {
    startOffset = 0;
    nonSilentFramesToProcess = 0;

    playbackState_ = PlaybackState::STOP_SCHEDULED;
    handleStopScheduled();
    return;
  }

  if (isUnscheduled()) {
    startOffset = 0;
    nonSilentFramesToProcess = 0;
    return;
  }

  // normal "mid-buffer" playback
  startOffset = 0;
  nonSilentFramesToProcess = framesToProcess;
}

void AudioScheduledSourceNode::disable() {
  AudioNode::disable();

  auto onEndedCallbackId = onEndedCallbackId_.load(std::memory_order_acquire);
  if (onEndedCallbackId != 0) {
    audioEventHandlerRegistry_->invokeHandlerWithEventBody(
        AudioEvent::ENDED, onEndedCallbackId, {});
  }
}

void AudioScheduledSourceNode::handleStopScheduled() {
  if (isStopScheduled()) {
    playbackState_ = PlaybackState::FINISHED;
    disable();
  }
}

} // namespace audioapi
