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
  // Send a dummy command to unblock the daemon thread if it's waiting.
  // The command channel uses OVERWRITE_ON_FULL, so this never blocks.
  commandSender_.send(SeekRequest{0});
  if (seekDecoderThread_.joinable()) {
    seekDecoderThread_.join();
  }
}

bool AudioFileSourceNode::initDecoder(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioFileSourceOptions &options) {
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
      .filePath = options.filePath,
      .memoryData = options.data,
      .contextSampleRate = context->getSampleRate(),
      .loop = options.loop};

  seekDecoderDaemon_ = std::make_unique<SeekDecoderDaemon>(
      daemonOptions,
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

  return true;
}

void AudioFileSourceNode::setOnPositionChangedCallbackId(uint64_t callbackId) {
  onPositionChangedCallbackId_ = callbackId;
}

void AudioFileSourceNode::unregisterOnPositionChangedCallback(uint64_t callbackId) {
  audioEventHandlerRegistry_->unregisterHandler(AudioEvent::POSITION_CHANGED, callbackId);
}

void AudioFileSourceNode::sendOnPositionChangedEvent(int framesPlayed) {
  const bool flush = decoderState_->onPositionChangedFlush.load(std::memory_order_acquire);

  if (flush && !audioThreadSetFlush_) {
    // Daemon completed a seek or loop-back - sync audio thread's position to decoder's actual position
    currentTime_.store(
        decoderState_->currentTime.load(std::memory_order_acquire) +
            static_cast<double>(framesPlayed) / sampleRate_,
        std::memory_order_release);
  } else {
    currentTime_.fetch_add(static_cast<double>(framesPlayed) / sampleRate_);
  }

  if (onPositionChangedCallbackId_ != 0 &&
      (flush || onPositionChangedTime_ > onPositionChangedInterval_)) {
    audioEventHandlerRegistry_->dispatchEvent(
        AudioEvent::POSITION_CHANGED,
        onPositionChangedCallbackId_,
        DoubleValuePayload{.value = getCurrentTime()});

    onPositionChangedTime_ = 0;
    audioThreadSetFlush_ = false;
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
  stopDaemonThread();
  filePaused_ = false;

  AudioScheduledSourceNode::disable();
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

size_t AudioFileSourceNode::readInterleavedFrames(
    const std::shared_ptr<DSPAudioBuffer> &destBuffer,
    size_t framesToRead) {
  if (decoderState_->pendingOffloadedSeeks.load(std::memory_order_acquire) > 0) {
    return 0;
  }

  // Read from the decoder daemon thread via the SPSC channel
  DecoderData incoming;
  if (frameReceiver_->try_receive(incoming) == ResponseStatus::SUCCESS) {
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

  size_t framesRead = readInterleavedFrames(processingBuffer, offsetLength);
  sendOnPositionChangedEvent(static_cast<int>(framesRead));

  // Handle End of Stream / End of File boundaries
  if (framesRead < offsetLength) {
    if (decoderState_->isEof.load(std::memory_order_acquire) &&
        !decoderState_->loop.load(std::memory_order_acquire)) {

      currentTime_.store(duration_, std::memory_order_release);
      audioThreadSetFlush_ = true;
      decoderState_->onPositionChangedFlush.store(true, std::memory_order_release);
      sendOnPositionChangedEvent(static_cast<int>(offsetLength - framesRead));

      filePaused_ = true;
      playbackState_ = PlaybackState::STOP_SCHEDULED;
    }

    // fill the rest of the buffer with silence
    processingBuffer->zero(startOffset + framesRead, offsetLength - framesRead);
  }

  if (isStopScheduled()) {
    handleStopScheduled();
  }

  return processingBuffer;
}

} // namespace audioapi
