#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/libs/base64/base64.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>

#include <audioapi/libs/miniaudio/decoders/libopus/miniaudio_libopus.h>
#include <audioapi/libs/miniaudio/decoders/libvorbis/miniaudio_libvorbis.h>
#include <audioapi/libs/miniaudio/miniaudio.h>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

// Decoding audio in fixed-size chunks because total frame count can't be
// determined in advance. Note: ma_decoder_get_length_in_pcm_frames() always
// returns 0 for Vorbis decoders.
Result<std::vector<float>, std::string> AudioDecoder::readAllPcmFrames(
    ma_decoder &decoder,
    int outputChannels) {
  std::vector<float> buffer;
  std::vector<float> temp(CHUNK_SIZE * outputChannels);
  ma_uint64 outFramesRead = 0;

  while (true) {
    ma_uint64 tempFramesDecoded = 0;
    ma_decoder_read_pcm_frames(&decoder, temp.data(), CHUNK_SIZE, &tempFramesDecoded);
    if (tempFramesDecoded == 0) {
      break;
    }

    buffer.insert(buffer.end(), temp.data(), temp.data() + tempFramesDecoded * outputChannels);
    outFramesRead += tempFramesDecoded;
  }

  if (outFramesRead == 0) {
    return Err("Failed to decode any frames");
  }

  return Ok(std::move(buffer));
}

AudioBufferResult AudioDecoder::makeAudioBufferFromFloatBuffer(
    const std::vector<float> &buffer,
    float outputSampleRate,
    int outputChannels) {
  if (buffer.empty()) {
    return Err("Buffer is empty");
  }

  auto outputFrames = buffer.size() / outputChannels;
  auto audioBuffer = std::make_shared<AudioBuffer>(outputFrames, outputChannels, outputSampleRate);

  audioBuffer->deinterleaveFrom(buffer.data(), outputFrames);

  return Ok(std::move(audioBuffer));
}

AudioBufferResult AudioDecoder::decodeWithMiniaudio(float sampleRate, DecoderSource source) {
  ma_decoder decoder;
  ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, static_cast<int>(sampleRate));
  ma_decoding_backend_vtable *customBackends[] = {
      ma_decoding_backend_libvorbis, ma_decoding_backend_libopus};

  config.ppCustomBackendVTables = customBackends;
  config.customBackendCount = sizeof(customBackends) / sizeof(customBackends[0]);

  ma_result initResult = std::visit(
      [&config, &decoder](auto &&arg) -> ma_result {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, MemorySource>) {
          return ma_decoder_init_memory(arg.data, arg.size, &config, &decoder);
        } else if constexpr (std::is_same_v<T, std::string>) {
          return ma_decoder_init_file(arg.c_str(), &config, &decoder);
        } else {
          return MA_INVALID_ARGS;
        }
      },
      source);

  if (initResult != MA_SUCCESS) {
    return Err(
        "Failed to initialize miniaudio decoder: " +
        std::string(ma_result_description(initResult)));
  }

  auto outputSampleRate = static_cast<float>(decoder.outputSampleRate);
  auto outputChannels = static_cast<int>(decoder.outputChannels);

  auto result = readAllPcmFrames(decoder, outputChannels)
                    .and_then([outputSampleRate, outputChannels](std::vector<float> &&buffer) {
                      return makeAudioBufferFromFloatBuffer(
                          std::move(buffer), outputSampleRate, outputChannels);
                    });

  ma_decoder_uninit(&decoder);

  return result;
}

AudioBufferResult AudioDecoder::decodeWithFilePath(const std::string &path, float sampleRate) {
  if (AudioDecoder::pathHasExtension(path, {".mp4", ".m4a", ".aac"})) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    auto buffer = ffmpegdecoder::decodeWithFilePath(path, static_cast<int>(sampleRate));
    if (buffer == nullptr) {
      return Err("Failed to decode with file path using FFmpeg");
    }
    return Ok(std::move(buffer));
#else
    return Err("FFmpeg is disabled, cannot decode with file path");
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  }
  return decodeWithMiniaudio(sampleRate, path);
}

AudioBufferResult
AudioDecoder::decodeWithMemoryBlock(const void *data, size_t size, float sampleRate) {
  const AudioFormat format = AudioDecoder::detectAudioFormat(data, size);
  if (format == AudioFormat::MP4 || format == AudioFormat::M4A || format == AudioFormat::AAC) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    auto buffer = ffmpegdecoder::decodeWithMemoryBlock(data, size, static_cast<int>(sampleRate));
    if (buffer == nullptr) {
      return Err("Failed to decode with memory block using FFmpeg");
    }
    return Ok(std::move(buffer));
#else
    return Err("FFmpeg is disabled, cannot decode memory block");
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  }
  return decodeWithMiniaudio(sampleRate, MemorySource{.data = data, .size = size});
}

AudioBufferResult AudioDecoder::decodeWithPCMInBase64(
    const std::string &data,
    float inputSampleRate,
    int inputChannelCount,
    bool interleaved) {
  auto decodedData = base64_decode(data, false);
  const auto uint8Data = reinterpret_cast<uint8_t *>(decodedData.data());
  size_t numFramesDecoded = decodedData.size() / (inputChannelCount * sizeof(int16_t));

  auto audioBuffer =
      std::make_shared<AudioBuffer>(numFramesDecoded, inputChannelCount, inputSampleRate);

  for (size_t ch = 0; ch < inputChannelCount; ++ch) {
    auto channelData = audioBuffer->getChannel(ch)->span();

    for (size_t i = 0; i < numFramesDecoded; ++i) {
      size_t offset;
      if (interleaved) {
        // Ch1, Ch2, Ch1, Ch2, ...
        offset = (i * inputChannelCount + ch) * sizeof(int16_t);
      } else {
        // Ch1, Ch1, Ch1, ..., Ch2, Ch2, Ch2, ...
        offset = (ch * numFramesDecoded + i) * sizeof(int16_t);
      }

      channelData[i] = uint8ToFloat(uint8Data[offset], uint8Data[offset + 1]);
    }
  }

  return Ok(std::move(audioBuffer));
}

} // namespace audioapi