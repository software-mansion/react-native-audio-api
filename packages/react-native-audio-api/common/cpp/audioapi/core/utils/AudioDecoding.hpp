#pragma once

#include <audioapi/core/types/AudioFormat.h>
#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Result.hpp>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace audioapi::audiodecoding {

using AudioBufferResult = Result<std::shared_ptr<AudioBuffer>, std::string>;

[[nodiscard]] AudioBufferResult decodeWithFilePath(const std::string &path, float sampleRate);
[[nodiscard]] AudioBufferResult
decodeWithMemoryBlock(const void *data, size_t size, float sampleRate);
[[nodiscard]] AudioBufferResult decodeWithPCMInBase64(
    const std::string &data,
    float inputSampleRate,
    int inputChannelCount,
    bool interleaved);

[[nodiscard]] AudioFormat detectAudioFormat(const void *data, size_t size);

[[nodiscard]] bool pathHasExtension(
    const std::string &path,
    const std::vector<std::string> &extensions);

[[nodiscard]] inline bool needsFFmpeg(AudioFormat format) {
  return format == AudioFormat::MP4 || format == AudioFormat::M4A || format == AudioFormat::AAC;
}

[[nodiscard]] inline bool needsFFmpegByPath(const std::string &path) {
  return pathHasExtension(path, {".mp4", ".m4a", ".aac"});
}

[[nodiscard]] inline float uint8ToFloat(uint8_t byte1, uint8_t byte2) {
  return static_cast<float>(static_cast<int16_t>((byte2 << CHAR_BIT) | byte1)) / INT16_MAX;
}

template <typename D>
  requires std::is_base_of_v<decoding::IncrementalAudioDecoder, D>
std::optional<double> probeDuration(const void *data, size_t size, int outputSampleRate) {
  D decoder;
  const auto openResult = decoder.openMemory(outputSampleRate, data, size);
  if (openResult.is_err()) {
    return std::nullopt;
  }
  return static_cast<double>(decoder.getDurationInSeconds());
}

} // namespace audioapi::audiodecoding
