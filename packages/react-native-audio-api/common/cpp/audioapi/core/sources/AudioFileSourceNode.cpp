#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/types/NodeOptions.h>

#include <audioapi/core/AudioContext.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>

namespace audioapi {

AudioFileSourceNode::AudioFileSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioFileSourceOptions &options)
    : AudioNode(context, options),
      audioEventHandlerRegistry_(context->getAudioEventHandlerRegistry()),
      onPositionChangedInterval_(
          static_cast<int>(context->getSampleRate() * ON_POSITION_CHANGED_INTERVAL)),
      requiresFFmpeg_(options.requiresFFmpeg) {
  volume_.store(options.volume, std::memory_order_release);
  loop_.store(options.loop, std::memory_order_release);
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

void AudioFileSourceNode::setOnEndedCallbackId(uint64_t callbackId) {
  onEndedCallbackId_ = callbackId;
}

void AudioFileSourceNode::unregisterOnEndedCallback(uint64_t callbackId) {
  audioEventHandlerRegistry_->unregisterHandler(AudioEvent::ENDED, callbackId);
}

void AudioFileSourceNode::sendOnPositionChangedEvent(int samplesWritten) {
  currentTime_.fetch_add(samplesWritten / sampleRate_);
  if (onPositionChangedCallbackId_ != 0 &&
      (onPositionChangedFlush_.load(std::memory_order_acquire) ||
       onPositionChangedTime_ > onPositionChangedInterval_)) {
    std::unordered_map<std::string, EventValue> body = {{"value", getCurrentTime()}};

    audioEventHandlerRegistry_->invokeHandlerWithEventBody(
        AudioEvent::POSITION_CHANGED, onPositionChangedCallbackId_, body);

    onPositionChangedTime_ = 0;
    onPositionChangedFlush_.store(false, std::memory_order_release);
  }

  onPositionChangedTime_ += samplesWritten;
}

void AudioFileSourceNode::sendOnEndedEvent() {
  if (onEndedCallbackId_ != 0) {
    audioEventHandlerRegistry_->invokeHandlerWithEventBody(
        AudioEvent::ENDED, onEndedCallbackId_, {});
  }
}

void AudioFileSourceNode::initDecoders(
    bool useFilePath,
    const std::shared_ptr<BaseAudioContext> &context,
    const std::shared_ptr<AudioFileDecoderState> &state) {
  bool ok = false;
  if (requiresFFmpeg_) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    decoder_ = std::make_unique<ffmpegdecoder::FFmpegDecoder>();
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  } else {
    decoder_ = std::make_unique<miniaudio_decoder::MiniAudioDecoder>();
  }
  if (useFilePath) {
    ok = decoder_->openFile(context->getSampleRate(), state->filePath);
  } else {
    ok = decoder_->openMemory(
        context->getSampleRate(), state->memoryData.data(), state->memoryData.size());
  }
  if (ok) {
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
}

// Same as AudioScheduledSourceNode::start: start or resume the native engine
void AudioFileSourceNode::start() {
  filePaused_.store(false, std::memory_order_release);

  if (std::shared_ptr<BaseAudioContext> ctx = context_.lock();
      auto *audioContext = dynamic_cast<AudioContext *>(ctx.get())) {
    if (audioContext->getState() != ContextState::RUNNING) {
      audioContext->start();
    }
  }
}

void AudioFileSourceNode::pause() {
  filePaused_.store(true, std::memory_order_release);
}

void AudioFileSourceNode::disable() {
  seekOffloader_.reset();
  filePaused_.store(false, std::memory_order_release);
  decoder_->close();
}

size_t AudioFileSourceNode::readFrames(float *buf, size_t frameCount) {
  if (pendingOffloadedSeeks_.load(std::memory_order_acquire) > 0) {
    return 0;
  }
  return decoder_->readPcmFrames(buf, frameCount);
}

bool AudioFileSourceNode::seekDecoderToTime(double seconds) {
  return decoder_->seekToTime(seconds);
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
  if (seekDecoderToTime(req.seconds)) {
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

void AudioFileSourceNode::writeInterleavedToBuffer(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    const AudioFileDecoderState &state,
    size_t destSampleOffset,
    size_t frameCount,
    float vol) {
  if (vol == 0) {
    processingBuffer->zero();
    return;
  }
  processingBuffer->deinterleaveFrom(state.interleavedBuffer.data(), frameCount);
  processingBuffer->scale(vol);
}

size_t AudioFileSourceNode::handleEof(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    size_t framesToProcess,
    size_t framesRead,
    float vol) {
  if (!loop_.load(std::memory_order_acquire)) {
    return framesRead;
  }

  if (!seekDecoderToTime(0)) {
    return framesRead;
  }

  size_t toFill = framesToProcess - framesRead;
  if (toFill == 0) {
    return framesRead;
  }

  auto &state = *decoderState_;
  size_t extra = readFrames(state.interleavedBuffer.data(), toFill);

  if (vol != 0) {
    writeInterleavedToBuffer(processingBuffer, state, framesRead, extra, vol);
  }

  return framesRead + extra;
}

std::shared_ptr<DSPAudioBuffer> AudioFileSourceNode::processNode(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (decoderState_ == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (pendingOffloadedSeeks_.load(std::memory_order_acquire) > 0) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (filePaused_.load(std::memory_order_acquire)) {
    processingBuffer->zero();
    return processingBuffer;
  }

  auto &state = *decoderState_;

  size_t framesRead = readFrames(state.interleavedBuffer.data(), framesToProcess);
  sendOnPositionChangedEvent(static_cast<int>(framesRead));

  const float vol = volume_.load(std::memory_order_acquire);
  writeInterleavedToBuffer(processingBuffer, state, 0, framesRead, vol);

  if (framesRead < framesToProcess) {
    if (!loop_.load(std::memory_order_acquire)) {
      sendOnEndedEvent();
      // if duration is not properly estimated, skip to the end
      currentTime_.store(duration_, std::memory_order_release);
      onPositionChangedFlush_.store(true, std::memory_order_release);
      sendOnPositionChangedEvent(static_cast<int>(framesToProcess - framesRead));
      filePaused_.store(true);
      processingBuffer->zero(framesRead, framesToProcess - framesRead);
      return processingBuffer;
    }
    size_t totalFilled = handleEof(processingBuffer, framesToProcess, framesRead, vol);
    onPositionChangedFlush_.store(true, std::memory_order_release);
    sendOnPositionChangedEvent(static_cast<int>(totalFilled));
    processingBuffer->zero(totalFilled, framesToProcess - totalFilled);
  }

  return processingBuffer;
}

} // namespace audioapi
