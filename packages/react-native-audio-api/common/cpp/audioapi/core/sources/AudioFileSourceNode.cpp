#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/types/NodeOptions.h>

#include <audioapi/core/AudioContext.h>

#include <audioapi/core/utils/decoding/SeekDecoderDaemon.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED

namespace audioapi {

AudioFileSourceNode::AudioFileSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioFileSourceOptions &options)
    : AudioScheduledSourceNode(context, options),
      decoderState_(std::make_shared<AudioFileDecoderState>()),
      volume_(options.volume),
      loop_(options.loop),
      onPositionChangedInterval_(
          static_cast<int>(context->getSampleRate() * ON_POSITION_CHANGED_INTERVAL)) {
  const bool useFilePath = !options.filePath.empty();
  const bool useData = !options.data.empty();

  // TODO: possibly check for file format, if hls and ffmpeg is not available, then fail to initialize
  // ->> add to options parsing validation

  if (!useFilePath && !useData) {
    assert(false && "AudioFileSourceNode requires either a file path or memory data to initialize");
    return;
  }

  // TODO: possibly move to an initialization function, and handle failure more gracefully
  auto [frameSender, frameReceiver] =
      channels::spsc::channel<DecoderData, FRAME_SPSC_OVERFLOW_STRATEGY, FRAME_SPSC_WAIT_STRATEGY>(
          FRAME_SPSC_CHANNEL_CAPACITY);
  frameReceiver_ = std::move(frameReceiver);

  auto [commandSender, commandReceiver] = channels::spsc::
      channel<SeekRequest, COMMAND_SPSC_OVERFLOW_STRATEGY, COMMAND_SPSC_WAIT_STRATEGY>(
          COMMAND_SPSC_CHANNEL_CAPACITY);
  commandSender_ = std::move(commandSender);

  SeekDecoderDaemonOptions daemonOptions{
      .requiresFFmpeg = options.requiresFFmpeg,
      .filePath = options.filePath,
      .memoryData = options.data,
      .contextSampleRate = context->getSampleRate(),
      .loop = options.loop};

  seekDecoderDaemon_ = std::make_unique<SeekDecoderDaemon>(
      daemonOptions, decoderState_, std::move(commandReceiver), std::move(frameSender));

  // TODO: check if all this is needed or some may be accessed directly from the daemon thread
  channelCount_ = decoderState_->channelCount;
  sampleRate_ = decoderState_->sampleRate;
  duration_ = decoderState_->duration;

  isInitialized_.store(true, std::memory_order_release);

  audioBuffer_ = std::make_shared<DSPAudioBuffer>(
      static_cast<size_t>(RENDER_QUANTUM_SIZE), channelCount_, context->getSampleRate());

  isInitialized_.store(true, std::memory_order_release);
}

void AudioFileSourceNode::setOnPositionChangedCallbackId(uint64_t callbackId) {
  onPositionChangedCallbackId_ = callbackId;
}

void AudioFileSourceNode::unregisterOnPositionChangedCallback(uint64_t callbackId) {
  audioEventHandlerRegistry_->unregisterHandler(AudioEvent::POSITION_CHANGED, callbackId);
}

void AudioFileSourceNode::sendOnPositionChangedEvent(int framesPlayed) {
  currentTime_.fetch_add(framesPlayed / sampleRate_);
  if (onPositionChangedCallbackId_ != 0 &&
      (decoderState_->onPositionChangedFlush.load(std::memory_order_acquire) ||
       onPositionChangedTime_ > onPositionChangedInterval_)) {
    audioEventHandlerRegistry_->dispatchEvent(
        AudioEvent::POSITION_CHANGED,
        onPositionChangedCallbackId_,
        DoubleValuePayload{.value = getCurrentTime()});

    onPositionChangedTime_ = 0;
    decoderState_->onPositionChangedFlush.store(false, std::memory_order_release);
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
  decoderState_->isDaemonRunning.store(false, std::memory_order_release);
  commandSender_.send(
      SeekRequest{0}); // send a dummy command to unblock the daemon thread if it's waiting
  if (seekDecoderThread_.joinable()) {
    seekDecoderThread_.join();
  }
  filePaused_ = false;

  AudioScheduledSourceNode::disable();
}

void AudioFileSourceNode::applyPlaybackStateAfterSuccessfulSeek(double seconds) {
  currentTime_.store(seconds, std::memory_order_release);
  decoderState_->onPositionChangedFlush.store(true, std::memory_order_release);
}

void AudioFileSourceNode::seekToTime(double seconds) {
  if (decoderState_ == nullptr) {
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

// TODO: `DecoderData` may be initialized once and then reused as size is stable (128 samples, the render quantum size),
// to avoid dynamic memory allocations on the audio thread.
size_t AudioFileSourceNode::readInterleavedFrames(
    const std::shared_ptr<DSPAudioBuffer> &destBuffer,
    size_t framesToRead) {

  // TODO: if the decoder daemon shares frameReceiver_ directly, it can flush the queue when a seek happens,
  // seekFlag will guard the audio thread from reading stale data during an active seek

  // If a seek is active, continuously drain the pipe and return silence
  if (decoderState_->pendingOffloadedSeeks.load(std::memory_order_acquire) > 0) {
    DecoderData drop;
    while (frameReceiver_.try_receive(drop) == ResponseStatus::SUCCESS) {}
    return 0;
  }

  // Read from the decoder daemon thread via the SPSC channel.
  DecoderData incoming;
  if (frameReceiver_.try_receive(incoming) == ResponseStatus::SUCCESS) {
    size_t framesToCopy = std::min(framesToRead, incoming.size);
    destBuffer->deinterleaveFrom(incoming.interleavedBuffer.data(), framesToCopy);

    if (volume_ != 1.0f && framesToCopy > 0) {
      destBuffer->scale(volume_);
    }

    return framesToCopy;
  }

  return 0;
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

  // TODO: either handle seek here on in reading frames, no need to duplicate the logic,
  // remove pause as it is already handled above

  // Handle running sync gate blocks or user pause interactions instantly
  if (decoderState_->pendingOffloadedSeeks.load(std::memory_order_acquire) > 0 || filePaused_) {
    processingBuffer->zero();
    return processingBuffer;
  }

  // Web Audio Timeline calculations
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

  // Zero out optional leading sub-quantum scheduling padding space
  if (startOffset > 0) {
    processingBuffer->zero(0, startOffset);
  }

  // Consume, deinterleave, and copy chunks safely across the thread line
  size_t framesRead = readInterleavedFrames(processingBuffer, offsetLength);
  sendOnPositionChangedEvent(static_cast<int>(framesRead));

  // TODO: make eof/eos more explicit

  // Handle End of Stream / End of File boundaries
  if (framesRead < offsetLength) {
    if (decoderState_->isEof.load(std::memory_order_acquire) &&
        !decoderState_->loop.load(std::memory_order_acquire)) {

      currentTime_.store(duration_, std::memory_order_release);
      decoderState_->onPositionChangedFlush.store(true, std::memory_order_release);
      sendOnPositionChangedEvent(static_cast<int>(offsetLength - framesRead));

      filePaused_ = true;
      playbackState_ = PlaybackState::STOP_SCHEDULED;
    }

    // Isolate hardware drivers completely from trailing garbage stack noise
    processingBuffer->zero(startOffset + framesRead, offsetLength - framesRead);
  }

  if (isStopScheduled()) {
    handleStopScheduled();
  }

  return processingBuffer;
}

} // namespace audioapi
