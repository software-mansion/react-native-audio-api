#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/types/NodeOptions.h>

#include <audioapi/core/AudioContext.h>

#include <algorithm>
#include <cassert>
#include <memory>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED

namespace audioapi {

AudioFileSourceNode::AudioFileSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioFileSourceOptions &options)
    : AudioScheduledSourceNode(context, options),
      volume_(options.volume),
      requiresFFmpeg_(options.requiresFFmpeg),
      loop_(options.loop),
      onPositionChangedInterval_(
          static_cast<int>(context->getSampleRate() * ON_POSITION_CHANGED_INTERVAL)) {
  const bool useFilePath = !options.filePath.empty();
  const bool useData = !options.data.empty();

  if (useFilePath || useData) {
    auto state = std::make_shared<AudioFileDecoderState>();
    if (useData) {
      state->memoryData = options.data;
    }
    if (useFilePath) {
      state->filePath = options.filePath;
    }
    initDecoders(useFilePath, context, state);
  }

  if (decoderState_ == nullptr) {
    assert(false && "cannot initialize decoder");
    return;
  }

  seekOffloader_ = std::make_unique<task_offloader::TaskOffloader<
      OffloadedSeekRequest,
      spsc::OverflowStrategy::OVERWRITE_ON_FULL,
      spsc::WaitStrategy::ATOMIC_WAIT>>(
      SEEK_OFFLOADER_WORKER_COUNT, [this](OffloadedSeekRequest req) { runOffloadedSeekTask(req); });

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
      (onPositionChangedFlush_.load(std::memory_order_acquire) ||
       onPositionChangedTime_ > onPositionChangedInterval_)) {
    audioEventHandlerRegistry_->dispatchEvent(
        AudioEvent::POSITION_CHANGED,
        onPositionChangedCallbackId_,
        DoubleValuePayload{.value = getCurrentTime()});

    onPositionChangedTime_ = 0;
    onPositionChangedFlush_.store(false, std::memory_order_release);
  }

  onPositionChangedTime_ += framesPlayed;
}

void AudioFileSourceNode::initDecoders(
    bool useFilePath,
    const std::shared_ptr<BaseAudioContext> &context,
    const std::shared_ptr<AudioFileDecoderState> &state) {
  decoding::DecoderResult openResult = Ok(None);
  if (requiresFFmpeg_) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    decoder_ = std::make_unique<ffmpegdecoder::FFmpegDecoder>();
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  } else {
    decoder_ = std::make_unique<miniaudio_decoder::MiniAudioDecoder>();
  }
  if (useFilePath) {
    openResult = decoder_->openFile(static_cast<int>(context->getSampleRate()), state->filePath);
  } else {
    openResult = decoder_->openMemory(
        static_cast<int>(context->getSampleRate()),
        state->memoryData.data(),
        state->memoryData.size());
  }
  if (openResult.is_ok()) {
    state->channels = decoder_->outputChannels();
    state->sampleRate = static_cast<float>(decoder_->outputSampleRate());
    duration_ = static_cast<double>(decoder_->getDurationInSeconds());
  } else {
    decoder_->close();
  }
  state->interleavedBuffer.resize(static_cast<size_t>(RENDER_QUANTUM_SIZE) * state->channels);
  decoderState_ = state;
  channelCount_ = decoderState_->channels;
  sampleRate_ = decoderState_->sampleRate;
  audioBuffer_ = std::make_shared<DSPAudioBuffer>(
      static_cast<size_t>(RENDER_QUANTUM_SIZE), channelCount_, context->getSampleRate());
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
  seekOffloader_.reset();
  filePaused_ = false;
  if (decoder_ != nullptr) {
    decoder_->close();
  }
  AudioScheduledSourceNode::disable();
}

size_t AudioFileSourceNode::readFrames(float *buf, size_t frameCount) {
  if (pendingOffloadedSeeks_.load(std::memory_order_acquire) > 0) {
    return 0;
  }
  return decoder_->readPcmFrames(buf, frameCount);
}

void AudioFileSourceNode::applyPlaybackStateAfterSuccessfulSeek(double seconds) {
  currentTime_.store(seconds, std::memory_order_release);
  onPositionChangedFlush_.store(true, std::memory_order_release);
}

void AudioFileSourceNode::runOffloadedSeekTask(OffloadedSeekRequest req) {
  if (decoderState_ == nullptr) {
    pendingOffloadedSeeks_.fetch_sub(1, std::memory_order_acq_rel);
    return;
  }
  if (decoder_->seekToTime(req.seconds).is_ok()) {
    applyPlaybackStateAfterSuccessfulSeek(req.seconds);
  }
  pendingOffloadedSeeks_.fetch_sub(1, std::memory_order_acq_rel);
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
  pendingOffloadedSeeks_.fetch_add(1, std::memory_order_acq_rel);
  seekOffloader_->getSender()->send(OffloadedSeekRequest{seconds});
}

size_t AudioFileSourceNode::handleEof(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    size_t regionFrames,
    size_t framesRead,
    size_t destFrameOffset) {
  if (!loop_) {
    return framesRead;
  }

  if (!decoder_->seekToTime(0).is_ok()) {
    return framesRead;
  }

  const size_t toFill = regionFrames - framesRead;
  if (toFill == 0) {
    return framesRead;
  }

  auto &state = *decoderState_;
  const size_t extra = readFrames(state.interleavedBuffer.data(), toFill);

  if (volume_ != 0.0f) {
    processingBuffer->deinterleaveFrom(state.interleavedBuffer.data(), extra);
    processingBuffer->scale(volume_);
  }

  return framesRead + extra;
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
  if (decoderState_ == nullptr || decoder_ == nullptr || !decoder_->isOpen() || filePaused_) {
    processingBuffer->zero();
    return processingBuffer;
  }

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (pendingOffloadedSeeks_.load(std::memory_order_acquire) > 0) {
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

  auto &state = *decoderState_;

  size_t framesRead = readFrames(state.interleavedBuffer.data(), offsetLength);
  sendOnPositionChangedEvent(static_cast<int>(framesRead));

  if (volume_ != 0.0f && framesRead > 0) {
    processingBuffer->deinterleaveFrom(state.interleavedBuffer.data(), framesRead);
    processingBuffer->scale(volume_);
  }

  if (framesRead < offsetLength) {
    if (!loop_) {
      currentTime_.store(duration_, std::memory_order_release);
      onPositionChangedFlush_.store(true, std::memory_order_release);
      sendOnPositionChangedEvent(static_cast<int>(offsetLength - framesRead));
      filePaused_ = true;
      playbackState_ = PlaybackState::STOP_SCHEDULED;
      processingBuffer->zero(startOffset + framesRead, offsetLength - framesRead);
    } else {
      const size_t totalFilled = handleEof(processingBuffer, offsetLength, framesRead, startOffset);
      onPositionChangedFlush_.store(true, std::memory_order_release);
      currentTime_.store(0, std::memory_order_release);
      sendOnPositionChangedEvent(static_cast<int>(totalFilled));
      processingBuffer->zero(startOffset + totalFilled, offsetLength - totalFilled);
    }
  }

  if (isStopScheduled()) {
    handleStopScheduled();
  }

  return processingBuffer;
}

} // namespace audioapi
