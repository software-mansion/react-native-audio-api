#include <audioapi/core/utils/AudioDecoding.hpp>
#include <audioapi/libs/base64/base64.h>
#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
#include <audioapi/libs/miniaudio/MiniAudioDecoding.h>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi::audiodecoding {

// Drains an incremental decoder into an AudioBuffer. Total frame count is not
// known up front for some formats (e.g. Vorbis), so we read in fixed-size
// chunks and grow the interleaved accumulator until the decoder reports EOF.
AudioBufferResult decodeAll(decoding::IncrementalAudioDecoder &decoder) {
  const int channels = std::max(1, decoder.outputChannels());
  const auto outputSampleRate = static_cast<float>(decoder.outputSampleRate());

  std::vector<float> interleaved;
  std::vector<float> chunk(
      decoding::IncrementalAudioDecoder::CHUNK_SIZE * static_cast<size_t>(channels));

  while (true) {
    const size_t framesRead =
        decoder.readPcmFrames(chunk.data(), decoding::IncrementalAudioDecoder::CHUNK_SIZE);
    if (framesRead == 0) {
      break;
    }
    interleaved.insert(
        interleaved.end(),
        chunk.begin(),
        chunk.begin() + static_cast<std::ptrdiff_t>(framesRead * static_cast<size_t>(channels)));
  }

  if (interleaved.empty()) {
    return Err("Failed to decode any frames");
  }

  const size_t outputFrames = interleaved.size() / static_cast<size_t>(channels);
  auto audioBuffer = std::make_shared<AudioBuffer>(outputFrames, channels, outputSampleRate);
  audioBuffer->deinterleaveFrom(interleaved.data(), outputFrames);
  return Ok(std::move(audioBuffer));
}

// NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
AudioFormat detectAudioFormat(const void *data, size_t size) {
  if (size < 12) {
    return AudioFormat::UNKNOWN;
  }
  const auto *bytes = static_cast<const unsigned char *>(data);

  // WAV/RIFF
  if (std::memcmp(bytes, "RIFF", 4) == 0 && std::memcmp(bytes + 8, "WAVE", 4) == 0) {
    return AudioFormat::WAV;
  }

  // OGG
  if (std::memcmp(bytes, "OggS", 4) == 0) {
    return AudioFormat::OGG;
  }

  // FLAC
  if (std::memcmp(bytes, "fLaC", 4) == 0) {
    return AudioFormat::FLAC;
  }

  // AAC starts with 0xFF 0xF1 or 0xFF 0xF9
  if (bytes[0] == 0xFF && (bytes[1] & 0xF6) == 0xF0) {
    return AudioFormat::AAC;
  }

  // MP3: "ID3" or 11-bit frame sync (0xFF 0xE0)
  if (std::memcmp(bytes, "ID3", 3) == 0) {
    return AudioFormat::MP3;
  }
  if (bytes[0] == 0xFF && (bytes[1] & 0xE0) == 0xE0) {
    return AudioFormat::MP3;
  }

  if (std::memcmp(bytes + 4, "ftyp", 4) == 0) {
    if (std::memcmp(bytes + 8, "M4A ", 4) == 0) {
      return AudioFormat::M4A;
    }
    if (std::memcmp(bytes + 8, "qt  ", 4) == 0) {
      return AudioFormat::MOV;
    }
    return AudioFormat::MP4;
  }
  return AudioFormat::UNKNOWN;
}
// NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)

bool pathHasExtension(const std::string &path, const std::vector<std::string> &extensions) {
  std::string pathLower = path;
  std::ranges::transform(pathLower, pathLower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return std::ranges::any_of(
      extensions, [&pathLower](const std::string &ext) { return pathLower.ends_with(ext); });
}

bool isValidDuration(float duration) {
  return duration > 0.0F && std::isfinite(duration);
}

AudioDurationResult resolveDurationFromDecoder(decoding::IncrementalAudioDecoder &decoder) {
  const float duration = decoder.getDurationInSeconds();
  if (!isValidDuration(duration)) {
    return Err("Audio duration metadata is unavailable");
  }

  return Ok(duration);
}

AudioBufferResult decodeWithFilePath(const std::string &path, float sampleRate) {
  const int sr = static_cast<int>(sampleRate);

  if (needsFFmpegByPath(path)) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    ffmpeg_decoder::FFmpegDecoder decoder;
    const auto openResult = decoder.openFile(sr, path);
    if (openResult.is_err()) {
      return Err("Failed to open file with FFmpeg decoder: " + openResult.unwrap_err());
    }
    auto result = decodeAll(decoder);
    decoder.close();
    return result;
#else
    return Err("FFmpeg is disabled, cannot decode with file path");
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  }

  miniaudio_decoder::MiniAudioDecoder decoder;
  const auto openResult = decoder.openFile(sr, path);
  if (openResult.is_err()) {
    return Err("Failed to open file with miniaudio decoder: " + openResult.unwrap_err());
  }
  auto result = decodeAll(decoder);
  decoder.close();
  return result;
}

AudioDurationResult getDurationWithFilePath(const std::string &path) {
  constexpr int useDecoderNativeSampleRate = 0;

  if (needsFFmpegByPath(path)) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    ffmpeg_decoder::FFmpegDecoder decoder;
    const auto openResult = decoder.openFile(useDecoderNativeSampleRate, path);
    if (openResult.is_err()) {
      return Err("Failed to open file with FFmpeg decoder: " + openResult.unwrap_err());
    }
    auto result = resolveDurationFromDecoder(decoder);
    decoder.close();
    return result;
#else
    return Err("FFmpeg is disabled, cannot inspect duration with file path");
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  }

  miniaudio_decoder::MiniAudioDecoder decoder;
  const auto openResult = decoder.openFile(useDecoderNativeSampleRate, path);
  if (openResult.is_err()) {
    return Err("Failed to open file with miniaudio decoder: " + openResult.unwrap_err());
  }
  auto result = resolveDurationFromDecoder(decoder);
  decoder.close();
  return result;
}

AudioBufferResult decodeWithMemoryBlock(const void *data, size_t size, float sampleRate) {
  const int sr = static_cast<int>(sampleRate);
  const AudioFormat format = detectAudioFormat(data, size);

  if (needsFFmpeg(format)) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    ffmpeg_decoder::FFmpegDecoder decoder;
    const auto openResult = decoder.openMemory(sr, data, size);
    if (openResult.is_err()) {
      return Err("Failed to open memory block with FFmpeg decoder: " + openResult.unwrap_err());
    }
    auto result = decodeAll(decoder);
    decoder.close();
    return result;
#else
    return Err("FFmpeg is disabled, cannot decode memory block");
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  }

  miniaudio_decoder::MiniAudioDecoder decoder;
  const auto openResult = decoder.openMemory(sr, data, size);
  if (openResult.is_err()) {
    return Err("Failed to open memory block with miniaudio decoder: " + openResult.unwrap_err());
  }
  auto result = decodeAll(decoder);
  decoder.close();
  return result;
}

AudioBufferResult decodeWithPCMInBase64(
    const std::string &data,
    float inputSampleRate,
    int inputChannelCount,
    bool interleaved) {
  auto decodedData = base64_decode(data, false);
  const auto *uint8Data = reinterpret_cast<uint8_t *>(decodedData.data());
  size_t numFramesDecoded = decodedData.size() / (inputChannelCount * sizeof(int16_t));

  auto audioBuffer =
      std::make_shared<AudioBuffer>(numFramesDecoded, inputChannelCount, inputSampleRate);

  for (int ch = 0; ch < inputChannelCount; ++ch) {
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

} // namespace audioapi::audiodecoding
