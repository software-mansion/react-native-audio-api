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
#include <utility>

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
      FFmpegNeeded_ = AudioDecoder::pathHasExtension(options.filePath, {".mp4", ".m4a", ".aac"});
    } else {
      auto format = AudioDecoder::detectAudioFormat(options.data.data(), options.data.size());
      FFmpegNeeded_ =
          format == AudioFormat::MP4 || format == AudioFormat::M4A || format == AudioFormat::AAC;
    }

    if (FFmpegNeeded_) {
#if RN_AUDIO_API_FFMPEG_DISABLED
      assert(false && "File codec is not supported when FFmpeg is disabled");
#else
      ffmpegdecoder::ffmpegDecoderConfigInit(&cfg, static_cast<int>(context->getSampleRate()));
      bool result;
      if (useFilePath) {
        result = decoder.openFile(cfg, options.filePath);
      } else {
        result = decoder.openMemory(cfg, state->memoryData.data(), state->memoryData.size());
      }
      if (result) {
        state->channels = decoder.outputChannels();
        state->sampleRate = static_cast<float>(decoder.outputSampleRate());
        state->interleavedBuffer.resize(RENDER_QUANTUM_SIZE * state->channels);
        decoderState_ = std::move(state);
        channelCount_ = decoderState_->channels;
      }
#endif // RN_AUDIO_API_FFMPEG_DISABLED
    } else {
      ma_decoder_config config = ma_decoder_config_init(
          ma_format_f32, 0, static_cast<ma_uint32>(context->getSampleRate()));
      ma_decoding_backend_vtable *customBackends[] = {
          ma_decoding_backend_libvorbis, ma_decoding_backend_libopus};
      config.ppCustomBackendVTables = customBackends;
      config.customBackendCount = sizeof(customBackends) / sizeof(customBackends[0]);

      ma_result result;
      if (useFilePath) {
        result = ma_decoder_init_file(options.filePath.c_str(), &config, &state->decoder);
      } else {
        result = ma_decoder_init_memory(
            state->memoryData.data(), state->memoryData.size(), &config, &state->decoder);
      }

      if (result == MA_SUCCESS) {
        state->channels = static_cast<int>(state->decoder.outputChannels);
        state->sampleRate = static_cast<float>(state->decoder.outputSampleRate);
        state->interleavedBuffer.resize(RENDER_QUANTUM_SIZE * state->channels);
        decoderState_ = std::move(state);
        channelCount_ = decoderState_->channels;
      }
    }
  }

  isInitialized_.store(true, std::memory_order_release);
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
  if (FFmpegNeeded_) {
    decoder.close();
  } else {
    ma_decoder_uninit(&decoderState_->decoder);
  }
  AudioScheduledSourceNode::disable();
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

  if (filePaused_.load(std::memory_order_acquire)) {
    processingBuffer->zero(startOffset, nonSilentFrames);
    return processingBuffer;
  }

  auto &state = *decoderState_;
  size_t framesReadCount = 0;
  if (FFmpegNeeded_) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    framesReadCount = decoder.readPcmFrames(state.interleavedBuffer.data(), nonSilentFrames);
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  } else {
    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(
        &state.decoder, state.interleavedBuffer.data(), nonSilentFrames, &framesRead);
    framesReadCount = static_cast<size_t>(framesRead);
  }

  const float vol = volume_.load(std::memory_order_acquire);
  if (vol == 0) {
    processingBuffer->zero();
  } else {
    int numOutputChannels = processingBuffer->getNumberOfChannels();
    for (size_t i = 0; i < framesReadCount; i++) {
      for (int ch = 0; ch < numOutputChannels; ch++) {
        int srcCh = ch < state.channels ? ch : state.channels - 1;
        processingBuffer->getChannel(ch)->span()[startOffset + i] =
            vol * state.interleavedBuffer[i * state.channels + srcCh];
      }
    }

    if (framesReadCount < nonSilentFrames) {
      processingBuffer->zero(startOffset + framesReadCount, nonSilentFrames - framesReadCount);
      playbackState_ = PlaybackState::STOP_SCHEDULED;
    }
  }

  handleStopScheduled();
  return processingBuffer;
}

} // namespace audioapi
