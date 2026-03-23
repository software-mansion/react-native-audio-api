#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#include <audioapi/libs/miniaudio/decoders/libopus/miniaudio_libopus.h>
#include <audioapi/libs/miniaudio/decoders/libvorbis/miniaudio_libvorbis.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <audioapi/types/NodeOptions.h>

#if !RN_AUDIO_API_TEST
#include <audioapi/core/AudioContext.h>
#endif

#include <algorithm>
#include <cmath>
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
      onPositionChangedInterval_(static_cast<int>(context->getSampleRate() * 0.25f)) {
  const bool useFilePath = !options.filePath.empty();
  const bool useData = !options.data.empty();

  if (useFilePath || useData) {
    auto state = std::make_shared<AudioFileDecoderState>();
    if (useData) {
      state->memoryData = options.data;
    }
    if (useFilePath) {
      state->filePath = options.filePath;
      FFmpegNeeded_ = AudioDecoder::pathHasExtension(options.filePath, {".mp4", ".m4a", ".aac"});
    } else {
      auto format = AudioDecoder::detectAudioFormat(options.data.data(), options.data.size());
      FFmpegNeeded_ =
          format == AudioFormat::MP4 || format == AudioFormat::M4A || format == AudioFormat::AAC;
    }
    initDecoders(useFilePath, context, state);
  }

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
      (onPositionChangedFlush_ || onPositionChangedTime_ > onPositionChangedInterval_)) {
    std::unordered_map<std::string, EventValue> body = {{"value", getCurrentTime()}};

    audioEventHandlerRegistry_->invokeHandlerWithEventBody(
        AudioEvent::POSITION_CHANGED, onPositionChangedCallbackId_, body);

    onPositionChangedTime_ = 0;
    onPositionChangedFlush_ = false;
  }

  onPositionChangedTime_ += samplesWritten;
}

void AudioFileSourceNode::initDecoders(
    bool useFilePath,
    const std::shared_ptr<BaseAudioContext> &context,
    const std::shared_ptr<AudioFileDecoderState> &state) {
  if (FFmpegNeeded_) {
#if RN_AUDIO_API_FFMPEG_DISABLED
    assert(false && "File codec is not supported when FFmpeg is disabled");
#else
    ffmpegdecoder::ffmpegDecoderConfigInit(&cfg, static_cast<int>(context->getSampleRate()));
    bool result;
    if (useFilePath) {
      result = decoder.openFile(cfg, state->filePath);
    } else {
      result = decoder.openMemory(cfg, state->memoryData.data(), state->memoryData.size());
    }
    if (result) {
      state->channels = decoder.outputChannels();
      state->sampleRate = static_cast<float>(decoder.outputSampleRate());
      duration_.store(decoder.getDurationInSeconds(), std::memory_order_release);
    } else {
      decoder.close();
    }
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  } else {
    ma_decoder_config config =
        ma_decoder_config_init(ma_format_f32, 0, static_cast<ma_uint32>(context->getSampleRate()));
    ma_decoding_backend_vtable *customBackends[] = {
        ma_decoding_backend_libvorbis, ma_decoding_backend_libopus};
    config.ppCustomBackendVTables = customBackends;
    config.customBackendCount = sizeof(customBackends) / sizeof(customBackends[0]);

    maDecoder_ = std::make_unique<ma_decoder>();
    ma_result result;
    if (useFilePath) {
      result = ma_decoder_init_file(state->filePath.c_str(), &config, maDecoder_.get());
    } else {
      result = ma_decoder_init_memory(
          state->memoryData.data(), state->memoryData.size(), &config, maDecoder_.get());
    }

    if (result == MA_SUCCESS) {
      state->channels = static_cast<int>(maDecoder_->outputChannels);
      state->sampleRate = static_cast<float>(maDecoder_->outputSampleRate);
      ma_uint64 length = 0;
      if (ma_decoder_get_length_in_pcm_frames(maDecoder_.get(), &length) == MA_SUCCESS) {
        duration_.store(static_cast<double>(length) / state->sampleRate, std::memory_order_release);
      }
    } else {
      ma_decoder_uninit(maDecoder_.get());
      maDecoder_.reset();
    }
  }
  state->interleavedBuffer.resize(static_cast<size_t>(RENDER_QUANTUM_SIZE) * state->channels);
  decoderState_ = state;
  channelCount_ = decoderState_->channels;
  sampleRate_ = decoderState_->sampleRate;
}

// Same as AudioScheduledSourceNode::start: start or resume the native engine
void AudioFileSourceNode::start() {
  if (filePaused_.load(std::memory_order_acquire)) {
    filePaused_.store(false, std::memory_order_release);
    if (fileStarted_) {
      if (std::shared_ptr<BaseAudioContext> ctx = context_.lock();
          auto *audioContext = dynamic_cast<AudioContext *>(ctx.get())) {
        if (audioContext->getState() != ContextState::RUNNING) {
          audioContext->resume();
        }
      }
    } else if (std::shared_ptr<BaseAudioContext> ctx = context_.lock();
               auto *audioContext = dynamic_cast<AudioContext *>(ctx.get())) {
      if (audioContext->getState() != ContextState::RUNNING) {
        audioContext->start();
      }
    }
    return;
  }

  if (std::shared_ptr<BaseAudioContext> ctx = context_.lock()) {
    if (auto *audioContext = dynamic_cast<AudioContext *>(ctx.get())) {
      if (audioContext->getState() != ContextState::RUNNING) {
        audioContext->start();
      }
    }
  }
  fileStarted_ = true;
}

void AudioFileSourceNode::pause() {
  if (std::shared_ptr<BaseAudioContext> ctx = context_.lock()) {
    if (auto *audioContext = dynamic_cast<AudioContext *>(ctx.get())) {
      audioContext->suspend();
    }
  }

  filePaused_.store(true, std::memory_order_release);
}

void AudioFileSourceNode::disable() {
  filePaused_.store(false, std::memory_order_release);
  fileStarted_ = false;
  totalFramesRead_ = 0;
  if (FFmpegNeeded_) {
    decoder.close();
  } else if (maDecoder_ != nullptr) {
    ma_decoder_uninit(maDecoder_.get());
    maDecoder_.reset();
  }
}

size_t AudioFileSourceNode::readFrames(float *buf, size_t frameCount) {
  if (FFmpegNeeded_) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    return decoder.readPcmFrames(buf, frameCount);
#else
    return 0;
#endif
  }
  if (maDecoder_ == nullptr) {
    return 0;
  }
  ma_uint64 framesRead = 0;
  ma_decoder_read_pcm_frames(maDecoder_.get(), buf, frameCount, &framesRead);
  return static_cast<size_t>(framesRead);
}

bool AudioFileSourceNode::seekDecoderToStart() {
  bool seeked = false;
  if (FFmpegNeeded_) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    seeked = decoder.seekToTime(0);
#endif
  } else if (maDecoder_ != nullptr) {
    seeked = ma_decoder_seek_to_pcm_frame(maDecoder_.get(), 0) == MA_SUCCESS;
  }
  if (seeked) {
    totalFramesRead_ = 0;
  }
  return seeked;
}

void AudioFileSourceNode::seekToStart() {
  if (!seekDecoderToStart()) {
    return;
  }
  currentTime_.store(0);
  endedEventSent_ = false;
  onPositionChangedFlush_ = true;
}

bool AudioFileSourceNode::seekDecoderToTime(double seconds) {
  bool seeked = false;
  if (FFmpegNeeded_) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    seeked = decoder.seekToTime(seconds);
#endif
  } else if (maDecoder_ != nullptr && sampleRate_ > 0) {
    const auto frame = static_cast<ma_uint64>(std::llround(seconds * sampleRate_));
    seeked = ma_decoder_seek_to_pcm_frame(maDecoder_.get(), frame) == MA_SUCCESS;
  }
  if (seeked) {
    totalFramesRead_ = 0;
  }
  return seeked;
}

void AudioFileSourceNode::seekToTime(double seconds) {
  const double dur = duration_.load(std::memory_order_acquire);
  if (dur > 0) {
    seconds = std::clamp(seconds, 0.0, dur);
  } else {
    seconds = std::max(0.0, seconds);
  }
  if (!seekDecoderToTime(seconds)) {
    return;
  }
  currentTime_.store(seconds);
  endedEventSent_ = false;
  onPositionChangedFlush_ = true;
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
  auto numOutputChannels = static_cast<int>(processingBuffer->getNumberOfChannels());
  for (size_t i = 0; i < frameCount; i++) {
    for (int ch = 0; ch < numOutputChannels; ch++) {
      int srcCh = ch < state.channels ? ch : state.channels - 1;
      processingBuffer->getChannel(ch)->span()[destSampleOffset + i] =
          vol * state.interleavedBuffer[i * state.channels + srcCh];
    }
  }
}

size_t AudioFileSourceNode::handleEof(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    size_t framesToProcess,
    size_t framesRead,
    float vol) {
  if (!loop_.load(std::memory_order_acquire)) {
    return framesRead;
  }

  if (!seekDecoderToStart()) {
    return framesRead;
  }

  size_t toFill = framesToProcess - framesRead;
  if (toFill == 0) {
    return framesRead;
  }

  auto &state = *decoderState_;
  size_t extra = readFrames(state.interleavedBuffer.data(), toFill);
  totalFramesRead_ += extra;

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

  if (filePaused_.load(std::memory_order_acquire)) {
    processingBuffer->zero();
    return processingBuffer;
  }

  auto &state = *decoderState_;

  size_t framesRead = readFrames(state.interleavedBuffer.data(), framesToProcess);
  totalFramesRead_ += framesRead;
  sendOnPositionChangedEvent(static_cast<int>(framesRead));

  const float vol = volume_.load(std::memory_order_acquire);
  writeInterleavedToBuffer(processingBuffer, state, 0, framesRead, vol);

  if (framesRead < framesToProcess) {
    if (!loop_.load(std::memory_order_acquire)) {
      if (!endedEventSent_) {
        endedEventSent_ = true;
        if (onEndedCallbackId_ != 0) {
          audioEventHandlerRegistry_->invokeHandlerWithEventBody(
              AudioEvent::ENDED, onEndedCallbackId_, {});
        }
      }
      onPositionChangedFlush_ = true;
      sendOnPositionChangedEvent(static_cast<int>(framesToProcess - framesRead));
      processingBuffer->zero(framesRead, framesToProcess - framesRead);
      return processingBuffer;
    }
    size_t totalFilled = handleEof(processingBuffer, framesToProcess, framesRead, vol);
    onPositionChangedFlush_ = true;
    sendOnPositionChangedEvent(static_cast<int>(totalFilled));
    processingBuffer->zero(totalFilled, framesToProcess - totalFilled);
  }

  return processingBuffer;
}

} // namespace audioapi
