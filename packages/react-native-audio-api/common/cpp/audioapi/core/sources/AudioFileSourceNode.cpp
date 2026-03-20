#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#include <audioapi/libs/miniaudio/decoders/libopus/miniaudio_libopus.h>
#include <audioapi/libs/miniaudio/decoders/libvorbis/miniaudio_libvorbis.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <audioapi/types/NodeOptions.h>

#include <cstdio>
#include <memory>

namespace audioapi {

AudioFileSourceNode::AudioFileSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioFileSourceOptions &options)
    : AudioScheduledSourceNode(context, options) {
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
  state->interleavedBuffer.resize(RENDER_QUANTUM_SIZE * state->channels);
  decoderState_ = state;
  channelCount_ = decoderState_->channels;
  sampleRate_ = decoderState_->sampleRate;
}

void AudioFileSourceNode::setDecoderState(const std::shared_ptr<AudioFileDecoderState> &state) {
  decoderState_ = state;
  channelCount_ = state != nullptr ? state->channels : 1;
}

void AudioFileSourceNode::start(double when) {
  if (filePaused_.load(std::memory_order_acquire)) {
    filePaused_.store(false, std::memory_order_release);
    if (fileStarted_) {
      return;
    }
  }

  AudioScheduledSourceNode::start(when);
}

void AudioFileSourceNode::pause() {
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
  AudioScheduledSourceNode::disable();
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

bool AudioFileSourceNode::seekToStart() {
  bool seeked = false;
  if (FFmpegNeeded_) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    seeked = decoder.seekToStart();
#endif
  } else if (maDecoder_ != nullptr) {
    seeked = ma_decoder_seek_to_pcm_frame(maDecoder_.get(), 0) == MA_SUCCESS;
  }
  if (seeked) {
    totalFramesRead_ = 0;
    currentTime_.store(0.0, std::memory_order_release);
  }
  return seeked;
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
    size_t nonSilentFrames,
    size_t framesRead,
    float vol,
    size_t startOffset) {
  if (!loop_.load(std::memory_order_acquire)) {
    currentTime_.store(decoder.getDurationInSeconds(), std::memory_order_release);
    playbackState_ = PlaybackState::STOP_SCHEDULED;
    return framesRead;
  }

  if (!seekToStart()) {
    currentTime_.store(decoder.getDurationInSeconds(), std::memory_order_release);
    playbackState_ = PlaybackState::STOP_SCHEDULED;
    return framesRead;
  }

  playbackState_ = PlaybackState::PLAYING;

  size_t toFill = nonSilentFrames - framesRead;
  if (toFill == 0) {
    return framesRead;
  }

  auto &state = *decoderState_;
  size_t extra = readFrames(state.interleavedBuffer.data(), toFill);
  totalFramesRead_ += extra;
  if (sampleRate_ > 0) {
    currentTime_.store(
        static_cast<double>(totalFramesRead_) / sampleRate_, std::memory_order_release);
  }

  if (vol != 0) {
    writeInterleavedToBuffer(processingBuffer, state, startOffset + framesRead, extra, vol);
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

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }

  size_t startOffset = 0;
  size_t nonSilentFrames = 0;
  updatePlaybackInfo(
      processingBuffer,
      framesToProcess,
      startOffset,
      nonSilentFrames,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (!isPlaying() && !isStopScheduled()) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (startOffset > 0) {
    processingBuffer->zero(0, startOffset);
  }

  auto &state = *decoderState_;

  if (filePaused_.load(std::memory_order_acquire)) {
    processingBuffer->zero(startOffset, nonSilentFrames);
    return processingBuffer;
  }

  size_t framesRead = readFrames(state.interleavedBuffer.data(), nonSilentFrames);
  totalFramesRead_ += framesRead;
  if (sampleRate_ > 0) {
    currentTime_.store(
        static_cast<double>(totalFramesRead_) / sampleRate_, std::memory_order_release);
  }

  const float vol = volume_.load(std::memory_order_acquire);
  writeInterleavedToBuffer(processingBuffer, state, startOffset, framesRead, vol);

  if (framesRead < nonSilentFrames) {
    size_t totalFilled = handleEof(processingBuffer, nonSilentFrames, framesRead, vol, startOffset);
    processingBuffer->zero(startOffset + totalFilled, nonSilentFrames - totalFilled);
  }

  handleStopScheduled();
  return processingBuffer;
}

} // namespace audioapi
