#include <audioapi/core/utils/AudioDecoding.h>
#include <audioapi/decoding/IncrementalAudioDecoder.h>
#include <audioapi/decoding/OSDecoding.h>
#include <audioapi/libs/base64/base64.h>
#include <audioapi/libs/miniaudio/MiniAudioDecoding.h>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
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

bool pathHasExtension(const std::string &path, const std::vector<std::string> &extensions) {
  std::string pathLower = path;
  std::ranges::transform(pathLower, pathLower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return std::ranges::any_of(
      extensions, [&pathLower](const std::string &ext) { return pathLower.ends_with(ext); });
}

bool isHttpUrl(const std::string &path) {
  return path.starts_with("http://") || path.starts_with("https://");
}

bool isValidDuration(float duration) {
  return duration >= 0.0F && std::isfinite(duration);
}

AudioDurationResult probeDurationWithFilePath(const std::string &path) {
#if RN_AUDIO_API_HAS_OS_DECODER
  {
    os_decoder::Decoder platformDecoder;
    const auto platformOpen = platformDecoder.openFile(0, path);
    if (platformOpen.is_ok()) {
      auto result = resolveDurationFromDecoder(platformDecoder);
      platformDecoder.close();
      if (result.is_ok()) {
        return result;
      }
    } else {
      platformDecoder.close();
    }
  }
#endif // RN_AUDIO_API_HAS_OS_DECODER

  miniaudio_decoder::MiniAudioDecoder decoder;
  const auto openResult = decoder.openFile(0, path);
  if (openResult.is_err()) {
    return Err("Cannot read duration: file could not be decoded");
  }
  auto result = resolveDurationFromDecoder(decoder);
  decoder.close();
  if (result.is_err()) {
    return Err("Cannot read duration: file could not be decoded");
  }
  return result;
}

AudioDurationResult probeDurationWithMemory(const void *data, size_t size, int sampleRate) {
  const int sr = sampleRate != 0 ? sampleRate : 0;

#if RN_AUDIO_API_HAS_OS_DECODER
  {
    os_decoder::Decoder platformDecoder;
    const auto platformOpen = platformDecoder.openMemory(sr, data, size);
    if (platformOpen.is_ok()) {
      auto result = resolveDurationFromDecoder(platformDecoder);
      platformDecoder.close();
      if (result.is_ok()) {
        return result;
      }
    } else {
      platformDecoder.close();
    }
  }
#endif // RN_AUDIO_API_HAS_OS_DECODER

  miniaudio_decoder::MiniAudioDecoder decoder;
  const auto openResult = decoder.openMemory(sr, data, size);
  if (openResult.is_err()) {
    return Err("Cannot read duration: audio data could not be decoded");
  }
  auto result = resolveDurationFromDecoder(decoder);
  decoder.close();
  if (result.is_err()) {
    return Err("Cannot read duration: audio data could not be decoded");
  }
  return result;
}

AudioDurationResult probeDurationWithUrl(
    const std::string &url,
    int sampleRate,
    const std::map<std::string, std::string> &headers) {
  // Remote URL duration probing uses FFmpeg's HTTP/HLS demuxer (byte ranges).
#if !RN_AUDIO_API_FFMPEG_DISABLED
  ffmpeg_decoder::FFmpegDecoder decoder;
  const auto openResult = decoder.openUrl(sampleRate, url, headers);
  if (openResult.is_err()) {
    return Err("Failed to open URL with FFmpeg decoder: " + openResult.unwrap_err());
  }
  auto result = resolveDurationFromDecoder(decoder);
  decoder.close();
  return result;
#else
  (void)url;
  (void)sampleRate;
  (void)headers;
  return Err("FFmpeg is disabled, cannot probe duration from URL");
#endif // RN_AUDIO_API_FFMPEG_DISABLED
}

AudioBufferResult decodeWithFilePath(const std::string &path, float sampleRate) {
  const int sr = static_cast<int>(sampleRate);

#if RN_AUDIO_API_HAS_OS_DECODER
  {
    os_decoder::Decoder platformDecoder;
    const auto platformOpen = platformDecoder.openFile(sr, path);
    if (platformOpen.is_ok()) {
      auto result = decodeAll(platformDecoder);
      platformDecoder.close();
      if (result.is_ok()) {
        return result;
      }
    } else {
      platformDecoder.close();
    }
  }
#endif // RN_AUDIO_API_HAS_OS_DECODER

  miniaudio_decoder::MiniAudioDecoder decoder;
  const auto openResult = decoder.openFile(sr, path);
  if (openResult.is_err()) {
    return Err("Cannot decode file: unsupported or invalid audio format");
  }
  auto result = decodeAll(decoder);
  decoder.close();
  if (result.is_err()) {
    return Err("Cannot decode file: unsupported or invalid audio format");
  }
  return result;
}

AudioBufferResult decodeWithMemoryBlock(const void *data, size_t size, float sampleRate) {
  const int sr = static_cast<int>(sampleRate);

#if RN_AUDIO_API_HAS_OS_DECODER
  {
    os_decoder::Decoder platformDecoder;
    const auto platformOpen = platformDecoder.openMemory(sr, data, size);
    if (platformOpen.is_ok()) {
      auto result = decodeAll(platformDecoder);
      platformDecoder.close();
      if (result.is_ok()) {
        return result;
      }
    } else {
      platformDecoder.close();
    }
  }
#endif // RN_AUDIO_API_HAS_OS_DECODER

  miniaudio_decoder::MiniAudioDecoder decoder;
  const auto openResult = decoder.openMemory(sr, data, size);
  if (openResult.is_err()) {
    return Err("Cannot decode audio data: unsupported or invalid audio format");
  }
  auto result = decodeAll(decoder);
  decoder.close();
  if (result.is_err()) {
    return Err("Cannot decode audio data: unsupported or invalid audio format");
  }
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
