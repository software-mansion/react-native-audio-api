#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.hpp>
#if !RN_AUDIO_API_TEST
#include <audioapi/core/AudioContext.h>
#endif // RN_AUDIO_API_TEST

#include <algorithm>
#include <limits>
#include <memory>

namespace audioapi {

AudioScheduledSourceNode::AudioScheduledSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioScheduledSourceNodeOptions &options)
    : AudioNode(context, options),
      startTime_(-1.0),
      stopTime_(-1.0),
      playbackState_(PlaybackState::UNSCHEDULED),
      onEndedEvent_(context->getAudioEventHandlerRegistry(), context->getAudioEventProducer()) {}

void AudioScheduledSourceNode::start(double when) {
  playbackState_ = PlaybackState::SCHEDULED;
  startTime_ = when;
}

void AudioScheduledSourceNode::stop(double when) {
  if (isFinished()) {
    return;
  }

  stopTime_ = when;
  if (isPaused()) {
    // Nothing is playing, so there is no start to compare against: the node
    // finishes once `when` comes, unless resume() gets it playing before then.
    return;
  }

  auto context = context_.lock();
  auto startFrame = dsp::timeToSampleFrame(startTime_, context->getSampleRate());
  auto stopFrame = dsp::timeToSampleFrame(stopTime_, context->getSampleRate());
  // if stop is requested before start, mark as finished
  if (stopFrame <= startFrame) {
    playbackState_ = PlaybackState::FINISHED;
    markEnded();
    AudioNode::disable();
    // The node never plays, so the render path will not fire onended. Defer a
    // dispatch until the context clock reaches the requested stop time
    onEndedEvent_.deferEmpty(context->getDeferredEvents(), when);
    return;
  }
}

void AudioScheduledSourceNode::pause() {
  if (isScheduled() || isPlaying()) {
    playbackState_ = PlaybackState::PAUSED;
  }
}

void AudioScheduledSourceNode::resume(double when) {
  if (!isPaused()) {
    return;
  }

  if (stopTime_ != -1.0) {
    const auto context = context_.lock();
    const auto resumeTime = std::max(when, context ? context->getCurrentTime() : when);
    if (stopTime_ <= resumeTime) {
      return;
    }
  }

  // Not through the virtual start(): an override may treat it as a fresh start
  // and drop the stop time.
  playbackState_ = PlaybackState::SCHEDULED;
  startTime_ = when;
}

bool AudioScheduledSourceNode::isUnscheduled() const {
  return playbackState_ == PlaybackState::UNSCHEDULED;
}

bool AudioScheduledSourceNode::isScheduled() const {
  return playbackState_ == PlaybackState::SCHEDULED;
}

bool AudioScheduledSourceNode::isPlaying() const {
  return playbackState_ == PlaybackState::PLAYING;
}

bool AudioScheduledSourceNode::isPaused() const {
  return playbackState_ == PlaybackState::PAUSED;
}

bool AudioScheduledSourceNode::isFinished() const {
  return playbackState_ == PlaybackState::FINISHED;
}

bool AudioScheduledSourceNode::isStopScheduled() const {
  return playbackState_ == PlaybackState::STOP_SCHEDULED;
}

bool AudioScheduledSourceNode::hasEnded() const {
  return hasEnded_.load(std::memory_order_acquire);
}

void AudioScheduledSourceNode::markEnded() {
  hasEnded_.store(true, std::memory_order_release);
}

bool AudioScheduledSourceNode::canBeDestructed() const {
  // A paused node can only be resumed through its JS handle, so once nothing
  // holds it, it will never play again.
  return isUnscheduled() || isPaused() || isFinished();
}

std::optional<size_t> AudioScheduledSourceNode::getOutputChannelNumber() const {
  return outputChannelNumber_.load(std::memory_order_acquire);
}

void AudioScheduledSourceNode::updatePlaybackInfo(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess,
    size_t &startOffset,
    size_t &nonSilentFramesToProcess,
    float sampleRate,
    size_t currentSampleFrame) {
  auto firstFrame = currentSampleFrame;
  size_t lastFrame = firstFrame + framesToProcess - 1;
  size_t startFrame = startTime_ == -1.0
      ? firstFrame
      : std::max(dsp::timeToSampleFrame(startTime_, sampleRate), firstFrame);
  size_t stopFrame = stopTime_ == -1.0 ? std::numeric_limits<size_t>::max()
                                       : dsp::timeToSampleFrame(stopTime_, sampleRate);
  if (isFinished()) {
    startOffset = 0;
    nonSilentFramesToProcess = 0;
    return;
  }

  if (isPaused()) {
    startOffset = 0;
    nonSilentFramesToProcess = 0;
    finishIfStopFrameReached(stopFrame, lastFrame);
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
  markEnded();
  onEndedEvent_.dispatchEmptyFromAudioThread();
  AudioNode::disable();
}

void AudioScheduledSourceNode::advanceWithoutRender(std::size_t currentSampleFrame) {
  if (isUnscheduled() || isFinished() || stopTime_ == -1.0) {
    return;
  }

  const auto stopFrame = dsp::timeToSampleFrame(stopTime_, getContextSampleRate());
  if (stopFrame >= currentSampleFrame) {
    return;
  }

  playbackState_ = PlaybackState::STOP_SCHEDULED;
  handleStopScheduled();
}

void AudioScheduledSourceNode::assignOnEndedCallbackId(uint64_t callbackId) {
  onEndedEvent_.assignCallbackId(callbackId);
}

void AudioScheduledSourceNode::finishIfStopFrameReached(size_t stopFrame, size_t lastFrame) {
  if (stopFrame <= lastFrame) {
    playbackState_ = PlaybackState::STOP_SCHEDULED;
    handleStopScheduled();
  }
}

void AudioScheduledSourceNode::handleStopScheduled() {
  if (isStopScheduled()) {
    playbackState_ = PlaybackState::FINISHED;
    disable();
  }
}

} // namespace audioapi
