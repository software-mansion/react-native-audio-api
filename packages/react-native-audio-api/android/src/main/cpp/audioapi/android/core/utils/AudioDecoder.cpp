#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/libs/base64/base64.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

#define MINIAUDIO_IMPLEMENTATION
#include <audioapi/libs/miniaudio/decoders/libopus/miniaudio_libopus.h>
#include <audioapi/libs/miniaudio/decoders/libvorbis/miniaudio_libvorbis.h>
#include <audioapi/libs/miniaudio/miniaudio.h>

#ifndef AUDIO_API_TEST_SUITE
#include <android/log.h>
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif

namespace audioapi {

// Decoding audio in fixed-size chunks because total frame count can't be
// determined in advance. Note: ma_decoder_get_length_in_pcm_frames() always
// returns 0 for Vorbis decoders.
std::vector<int16_t> AudioDecoder::readAllPcmFrames(ma_decoder &decoder) const {
#ifndef AUDIO_API_TEST_SUITE
  std::vector<int16_t> buffer;
  std::vector<int16_t> temp(CHUNK_SIZE * numChannels_);
  ma_uint64 outFramesRead = 0;

  while (true) {
    ma_uint64 tempFramesDecoded = 0;
    ma_decoder_read_pcm_frames(
        &decoder, temp.data(), CHUNK_SIZE, &tempFramesDecoded);
    if (tempFramesDecoded == 0) {
      break;
    }

    buffer.insert(
        buffer.end(),
        temp.data(),
        temp.data() + tempFramesDecoded * numChannels_);
    outFramesRead += tempFramesDecoded;
  }

  if (outFramesRead == 0) {
    __android_log_print(ANDROID_LOG_ERROR, "AudioDecoder", "Failed to decode");
  }
  return buffer;
#else
  return nullptr;
#endif
}

std::shared_ptr<AudioBuffer> AudioDecoder::makeAudioBufferFromInt16Buffer(
    const std::vector<int16_t> &buffer) const {
  if (buffer.empty()) {
    return nullptr;
  }

  auto outputFrames = buffer.size() / numChannels_;
  auto audioBus =
      std::make_shared<AudioBus>(outputFrames, numChannels_, sampleRate_);

  for (int ch = 0; ch < numChannels_; ++ch) {
    auto channelData = audioBus->getChannel(ch)->getData();
    for (int i = 0; i < outputFrames; ++i) {
      channelData[i] = int16ToFloat(buffer[i * numChannels_ + ch]);
    }
  }
  return std::make_shared<AudioBuffer>(audioBus);
}

std::shared_ptr<AudioBuffer> AudioDecoder::decodeWithFilePath(
    const std::string &path) const {
  // if (path.starts_with("file://")) {
  //   path = path.replace(0, 7, "");
  // }
#ifndef AUDIO_API_TEST_SUITE
  std::vector<int16_t> buffer;
  if (AudioDecoder::pathHasExtension(path, {".mp4", ".m4a", ".aac"})) {
    buffer = ffmpegdecoding::decodeWithFilePath(
        path, numChannels_, static_cast<int>(sampleRate_));
    if (buffer.empty()) {
      __android_log_print(
          ANDROID_LOG_ERROR,
          "AudioDecoder",
          "Failed to decode with FFmpeg: %s",
          path.c_str());
      return nullptr;
    }
    return makeAudioBufferFromInt16Buffer(buffer);
  }
  ma_decoder decoder;
  ma_decoder_config config = ma_decoder_config_init(
      ma_format_s16, numChannels_, static_cast<int>(sampleRate_));
  ma_decoding_backend_vtable *customBackends[] = {
      ma_decoding_backend_libvorbis, ma_decoding_backend_libopus};

  config.ppCustomBackendVTables = customBackends;
  config.customBackendCount =
      sizeof(customBackends) / sizeof(customBackends[0]);

  if (ma_decoder_init_file(path.c_str(), &config, &decoder) != MA_SUCCESS) {
    __android_log_print(
        ANDROID_LOG_ERROR,
        "AudioDecoder",
        "Failed to initialize decoder for file: %s",
        path.c_str());
    ma_decoder_uninit(&decoder);
    return nullptr;
  }

  buffer = readAllPcmFrames(decoder);
  ma_decoder_uninit(&decoder);
  return makeAudioBufferFromInt16Buffer(buffer);
#else
  return nullptr;
#endif
}

std::shared_ptr<AudioBuffer> AudioDecoder::decodeWithMemoryBlock(
    const void *data,
    size_t size) const {
#ifndef AUDIO_API_TEST_SUITE
  std::vector<int16_t> buffer;
  const AudioFormat format = AudioDecoder::detectAudioFormat(data, size);
  if (format == AudioFormat::MP4 || format == AudioFormat::M4A ||
      format == AudioFormat::AAC) {
    buffer = ffmpegdecoding::decodeWithMemoryBlock(
        data, size, numChannels_, sampleRate_);
    if (buffer.empty()) {
      __android_log_print(
          ANDROID_LOG_ERROR, "AudioDecoder", "Failed to decode with FFmpeg");
      return nullptr;
    }
    return makeAudioBufferFromInt16Buffer(buffer);
  }
  ma_decoder decoder;
  ma_decoder_config config = ma_decoder_config_init(
      ma_format_s16, numChannels_, static_cast<int>(sampleRate_));

  ma_decoding_backend_vtable *customBackends[] = {
      ma_decoding_backend_libvorbis, ma_decoding_backend_libopus};

  config.ppCustomBackendVTables = customBackends;
  config.customBackendCount =
      sizeof(customBackends) / sizeof(customBackends[0]);

  if (ma_decoder_init_memory(data, size, &config, &decoder) != MA_SUCCESS) {
    __android_log_print(
        ANDROID_LOG_ERROR,
        "AudioDecoder",
        "Failed to initialize decoder for memory block");
    ma_decoder_uninit(&decoder);
    return nullptr;
  }

  buffer = readAllPcmFrames(decoder);
  ma_decoder_uninit(&decoder);
  return makeAudioBufferFromInt16Buffer(buffer);
#else
  return nullptr;
#endif
}

std::shared_ptr<AudioBuffer> AudioDecoder::decodeWithPCMInBase64(
    const std::string &data) const {
  auto decodedData = base64_decode(data, false);
  const auto uint8Data = reinterpret_cast<uint8_t *>(decodedData.data());
  size_t numFramesDecoded = decodedData.size() / 2;

  auto audioBus =
      std::make_shared<AudioBus>(numFramesDecoded, numChannels_, sampleRate_);
  auto leftChannelData = audioBus->getChannel(0)->getData();
  auto rightChannelData = audioBus->getChannel(1)->getData();

  for (size_t i = 0; i < numFramesDecoded; ++i) {
    float sample = int16ToFloat(
        static_cast<int16_t>((uint8Data[i * 2 + 1] << 8) | uint8Data[i * 2]));
    leftChannelData[i] = sample;
    rightChannelData[i] = sample;
  }

  return std::make_shared<AudioBuffer>(audioBus);
}

} // namespace audioapi
