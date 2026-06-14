#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <audioapi/core/AudioContext.h>

#include <audioapi/core/utils/decoding/SeekDecoderDaemon.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED

namespace audioapi {

AudioFileSourceNode::AudioFileSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    AudioFileSourceOptions &options)
    : AudioScheduledSourceNode(context, options),
      decoderState_(std::make_shared<AudioFileDecoderState>()),
      volume_(options.volume),
      stretch_(std::make_shared<signalsmith::stretch::SignalsmithStretch<float>>()),
      loop_(options.loop),
      onPositionChangedInterval_(
          static_cast<int>(context->getSampleRate() * ON_POSITION_CHANGED_INTERVAL)) {
  decoderState_->playbackRate.store(
      std::clamp(options.playbackRate, 0.5f, 2.0f), std::memory_order_release);
  decoderState_->preservesPitch.store(options.preservesPitch, std::memory_order_release);

  const bool useFilePath = !options.filePath.empty();
  const bool useData = !options.data.empty();

  if (!useFilePath && !useData) {
    assert(false && "AudioFileSourceNode requires either a file path or memory data to initialize");
    return;
  }

  if (!initDecoder(context, options)) {
    return;
  }

  isInitialized_.store(true, std::memory_order_release);
}

AudioFileSourceNode::~AudioFileSourceNode() {
  stopDaemonThread();
}

void AudioFileSourceNode::stopDaemonThread() {
  decoderState_->isDaemonRunning.store(false, std::memory_order_release);

  // commandSender_ is only created in initDecoder(); skip if construction failed early
  // (e.g. neither filePath nor data provided) or teardown already completed.
  if (!seekDecoderThread_.joinable() && seekDecoderDaemon_ == nullptr) {
    return;
  }

  // Send a dummy command to unblock the daemon thread if it's waiting.
  // The command channel uses OVERWRITE_ON_FULL, so this never blocks.
  commandSender_.send(SeekRequest{0});
  if (seekDecoderThread_.joinable()) {
    seekDecoderThread_.join();
  }
}

bool AudioFileSourceNode::initDecoder(
    const std::shared_ptr<BaseAudioContext> &context,
    AudioFileSourceOptions &options) {
  auto [frameSender, frameReceiver] =
      channels::spsc::channel<DecoderData, FRAME_OVERFLOW_STRATEGY, FRAME_WAIT_STRATEGY>(
          FRAME_CHANNEL_CAPACITY);
  frameReceiver_ = std::make_shared<FrameReceiver>(std::move(frameReceiver));

  auto [commandSender, commandReceiver] =
      channels::spsc::channel<SeekRequest, COMMAND_OVERFLOW_STRATEGY, COMMAND_WAIT_STRATEGY>(
          COMMAND_CHANNEL_CAPACITY);
  commandSender_ = std::move(commandSender);

  SeekDecoderDaemonOptions daemonOptions{
      .requiresFFmpeg = options.requiresFFmpeg,
      .filePath = std::move(options.filePath),
      .memoryData = std::move(options.data),
      .contextSampleRate = context->getSampleRate(),
      .loop = options.loop};

  seekDecoderDaemon_ = std::make_unique<SeekDecoderDaemon>(
      std::move(daemonOptions),
      decoderState_,
      std::move(commandReceiver),
      std::move(frameSender),
      frameReceiver_);

  if (!decoderState_->isReady.load(std::memory_order_acquire)) {
    return false;
  }

  channelCount_ = decoderState_->channelCount;
  sampleRate_ = decoderState_->sampleRate;
  duration_ = decoderState_->duration;

  audioBuffer_ = std::make_shared<DSPAudioBuffer>(
      static_cast<size_t>(RENDER_QUANTUM_SIZE), channelCount_, context->getSampleRate());
  playbackRateBuffer_ = std::make_shared<DSPAudioBuffer>(
      static_cast<size_t>(2 * RENDER_QUANTUM_SIZE), channelCount_, context->getSampleRate());
  stretch_->presetDefault(channelCount_, sampleRate_);

  return true;
}

void AudioFileSourceNode::setPlaybackRate(float v) {
  if (decoderState_ == nullptr) {
    return;
  }

  const float next = std::clamp(v, 0.5f, 2.0f);
  const float previous = decoderState_->playbackRate.exchange(next, std::memory_order_acq_rel);
  if (std::abs(previous - next) > 0.0001f) {
    handlePlaybackSettingsChanged();
  }
}

void AudioFileSourceNode::setPreservesPitch(bool v) {
  if (decoderState_ == nullptr) {
    return;
  }

  const bool previous = decoderState_->preservesPitch.exchange(v, std::memory_order_acq_rel);
  if (previous != v) {
    handlePlaybackSettingsChanged();
  }
}

void AudioFileSourceNode::drainPendingFrames() {
  if (frameReceiver_ == nullptr) {
    return;
  }

  DecoderData drop;
  while (frameReceiver_->try_receive(drop) == ResponseStatus::SUCCESS) {}
}

void AudioFileSourceNode::handlePlaybackSettingsChanged() {
  drainPendingFrames();
  if (stretch_ != nullptr) {
    stretch_->reset();
  }
  playbackFadeInRemainingFrames_ = PLAYBACK_TRANSITION_FADE_FRAMES;
}

void AudioFileSourceNode::setOnPositionChangedCallbackId(uint64_t callbackId) {
  onPositionChangedCallbackId_ = callbackId;
}

void AudioFileSourceNode::unregisterOnPositionChangedCallback(uint64_t callbackId) {
  audioEventHandlerRegistry_->unregisterHandler(AudioEvent::POSITION_CHANGED, callbackId);
}

void AudioFileSourceNode::sendOnPositionChangedEvent(int framesPlayed, bool forceFlush) {
  if (!forceFlush) {
    currentTime_.fetch_add(static_cast<double>(framesPlayed) / sampleRate_);
  }

  if (onPositionChangedCallbackId_ != 0 &&
      (forceFlush || onPositionChangedTime_ > onPositionChangedInterval_)) {

    audioEventHandlerRegistry_->dispatchEvent(
        AudioEvent::POSITION_CHANGED,
        onPositionChangedCallbackId_,
        DoubleValuePayload{.value = getCurrentTime()});

    onPositionChangedTime_ = 0;
  }

  onPositionChangedTime_ += framesPlayed;
}

void AudioFileSourceNode::connect(const std::shared_ptr<AudioNode> &node) {
  if (isRoutedThroughMediaElement()) {
    return;
  }

  AudioScheduledSourceNode::connect(node);
}

void AudioFileSourceNode::start(double when) {
  if (!isRoutedThroughMediaElement()) {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      connect(context->getDestination());
    }
  }

  AudioScheduledSourceNode::start(when);
  filePaused_ = false;

  if (seekDecoderDaemon_) {
    seekDecoderThread_ = std::thread(std::move(*seekDecoderDaemon_));
    seekDecoderDaemon_.reset();
  }
}

void AudioFileSourceNode::bindMediaElementSource(uint64_t bindingId) {
  activeMediaBindingId_.store(bindingId, std::memory_order_release);
}

void AudioFileSourceNode::releaseMediaElementSource(uint64_t bindingId) {
  uint64_t expected = bindingId;
  if (!activeMediaBindingId_.compare_exchange_strong(
          expected, 0, std::memory_order_acq_rel, std::memory_order_acquire)) {
    return;
  }

  ensureConnectedForDirectPlayback();
}

void AudioFileSourceNode::ensureConnectedForDirectPlayback() {
  if (filePaused_ || isUnscheduled() || isFinished()) {
    return;
  }

  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    connect(context->getDestination());
  }
}

bool AudioFileSourceNode::isCurrentMediaElementSource(uint64_t bindingId) const {
  if (bindingId == 0) {
    return false;
  }

  return activeMediaBindingId_.load(std::memory_order_acquire) == bindingId;
}

void AudioFileSourceNode::pause() {
  filePaused_ = true;
}

void AudioFileSourceNode::disable() {
  stopDaemonThread();
  filePaused_ = false;

  AudioScheduledSourceNode::disable();
}

void AudioFileSourceNode::seekToTime(double seconds) {
  if (decoderState_ == nullptr || !isInitialized_.load(std::memory_order_acquire)) {
    return;
  }
  const double dur = duration_;
  if (dur > 0) {
    seconds = std::clamp(seconds, 0.0, dur);
  } else {
    seconds = std::max(0.0, seconds);
  }
  decoderState_->pendingOffloadedSeeks.fetch_add(1, std::memory_order_acq_rel);
  commandSender_.send(SeekRequest{seconds});
}

bool AudioFileSourceNode::readNextFrameChunk(DecoderData &outData) {
  if (decoderState_->pendingOffloadedSeeks.load(std::memory_order_acquire) > 0) {
    return false;
  }
  return frameReceiver_->try_receive(outData) == ResponseStatus::SUCCESS;
}

void AudioFileSourceNode::renderWithPitchPreservation(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    const DecoderData &incoming,
    int framesToProcess) {
  if (stretch_ == nullptr || playbackRateBuffer_ == nullptr || incoming.size == 0) {
    processingBuffer->zero();
    return;
  }

  playbackRateBuffer_->zero();
  playbackRateBuffer_->deinterleaveFrom(incoming.interleavedBuffer.data(), incoming.size);
  playbackRateBuffer_->scale(volume_);

  processingBuffer->zero();
  stretch_->process(
      playbackRateBuffer_.get()[0],
      static_cast<int>(incoming.size),
      processingBuffer.get()[0],
      framesToProcess);
}

void AudioFileSourceNode::renderWithoutPitchPreservation(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    const DecoderData &incoming,
    int framesToProcess) {
  if (playbackRateBuffer_ == nullptr || incoming.size == 0 || framesToProcess <= 0) {
    processingBuffer->zero();
    return;
  }

  playbackRateBuffer_->zero();
  playbackRateBuffer_->deinterleaveFrom(incoming.interleavedBuffer.data(), incoming.size);
  processingBuffer->zero();

  const auto sourceFrames = incoming.size;
  const auto outputFrames = static_cast<size_t>(framesToProcess);
  const float step = sourceFrames > 1 && outputFrames > 1
      ? static_cast<float>(sourceFrames - 1) / static_cast<float>(outputFrames - 1)
      : 0.0f;

  for (size_t channel = 0; channel < static_cast<size_t>(channelCount_); ++channel) {
    const float *input = playbackRateBuffer_->getChannel(channel)->begin();
    float *output = processingBuffer->getChannel(channel)->begin();

    for (size_t frame = 0; frame < outputFrames; ++frame) {
      const float position = static_cast<float>(frame) * step;
      const auto index = static_cast<size_t>(position);
      const auto nextIndex = std::min(index + 1, sourceFrames - 1);
      const float fraction = position - static_cast<float>(index);
      output[frame] = (input[index] + (input[nextIndex] - input[index]) * fraction) * volume_;
    }
  }
}

void AudioFileSourceNode::applyPlaybackTransitionFade(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (playbackFadeInRemainingFrames_ <= 0 || framesToProcess <= 0) {
    return;
  }

  const int framesToFade = std::min(framesToProcess, playbackFadeInRemainingFrames_);
  const int fadeStart = PLAYBACK_TRANSITION_FADE_FRAMES - playbackFadeInRemainingFrames_;

  for (size_t channel = 0; channel < static_cast<size_t>(channelCount_); ++channel) {
    float *output = processingBuffer->getChannel(channel)->begin();
    for (int frame = 0; frame < framesToFade; ++frame) {
      const float gain = static_cast<float>(fadeStart + frame + 1) /
          static_cast<float>(PLAYBACK_TRANSITION_FADE_FRAMES);
      output[frame] *= gain;
    }
  }

  playbackFadeInRemainingFrames_ -= framesToFade;
}

std::shared_ptr<DSPAudioBuffer> AudioFileSourceNode::processNode(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (isRoutedThroughMediaElement()) {
    processingBuffer->zero();
    return processingBuffer;
  }
  return processDecodedOutput(processingBuffer, framesToProcess);
}

std::shared_ptr<DSPAudioBuffer> AudioFileSourceNode::processDecodedOutput(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (decoderState_ == nullptr || filePaused_) {
    processingBuffer->zero();
    return processingBuffer;
  }

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (decoderState_->pendingOffloadedSeeks.load(std::memory_order_acquire) > 0) {
    processingBuffer->zero();
    return processingBuffer;
  }

  size_t startOffset = 0;
  size_t offsetLength = 0;
  updatePlaybackInfo(
      processingBuffer,
      framesToProcess,
      startOffset,
      offsetLength,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (!isPlaying() && !isStopScheduled()) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (startOffset > 0) {
    processingBuffer->zero(0, startOffset);
  }

  DecoderData incoming;
  if (!readNextFrameChunk(incoming)) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (incoming.state == StreamState::END_OF_STREAM) {
    currentTime_.store(duration_, std::memory_order_release);
    sendOnPositionChangedEvent(0, true);

    filePaused_ = true;
    playbackState_ = PlaybackState::STOP_SCHEDULED;
    handleStopScheduled();

    processingBuffer->zero();
    return processingBuffer;
  }

  bool forceFlushEvent = false;
  if (incoming.state == StreamState::DISCONTINUOUS) {
    currentTime_.store(incoming.timestamp, std::memory_order_release);
    if (stretch_ != nullptr) {
      stretch_->reset();
    }
    forceFlushEvent = true;
  }

  const bool preservesPitch = decoderState_->preservesPitch.load(std::memory_order_acquire);
  const bool hasPlaybackRateChange = std::abs(incoming.playbackRate - 1.0f) > 0.0001f ||
      std::cmp_not_equal(incoming.size, framesToProcess);
  const bool shouldStretch = preservesPitch && hasPlaybackRateChange && stretch_ != nullptr &&
      playbackRateBuffer_ != nullptr && incoming.size > 0;
  const bool shouldResample = !preservesPitch && hasPlaybackRateChange &&
      playbackRateBuffer_ != nullptr && incoming.size > 0;

  size_t framesPlayed = std::min(static_cast<size_t>(framesToProcess), incoming.size);
  if (shouldStretch) {
    renderWithPitchPreservation(processingBuffer, incoming, framesToProcess);
  } else if (shouldResample) {
    renderWithoutPitchPreservation(processingBuffer, incoming, framesToProcess);
  } else {
    processingBuffer->deinterleaveFrom(incoming.interleavedBuffer.data(), framesPlayed);

    if (volume_ != 1.0f && framesPlayed > 0) {
      processingBuffer->scale(volume_);
    }
  }

  applyPlaybackTransitionFade(processingBuffer, framesToProcess);

  sendOnPositionChangedEvent(static_cast<int>(incoming.size), forceFlushEvent);

  // Fill tail end with silence if the chunk returned short
  if (!shouldStretch && !shouldResample && std::cmp_less(framesPlayed, framesToProcess)) {
    processingBuffer->zero(framesPlayed, framesToProcess - framesPlayed);
  }

  if (isStopScheduled()) {
    handleStopScheduled();
  }

  return processingBuffer;
}

} // namespace audioapi
