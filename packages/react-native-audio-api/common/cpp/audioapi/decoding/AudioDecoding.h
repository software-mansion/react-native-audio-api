#pragma once

#include <audioapi/decoding/backends/AudioDecoderBackend.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Result.hpp>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace audioapi::audiodecoding {

using AudioBufferResult = Result<std::shared_ptr<AudioBuffer>, std::string>;
using AudioDurationResult = Result<float, std::string>;

[[nodiscard]] AudioBufferResult decodeWithFilePath(const std::string &path, float sampleRate);
[[nodiscard]] AudioBufferResult
decodeWithMemoryBlock(const void *data, size_t size, float sampleRate);
[[nodiscard]] AudioBufferResult decodeWithPCMInBase64(
    const std::string &data,
    float inputSampleRate,
    int inputChannelCount,
    bool interleaved);

[[nodiscard]] bool pathHasExtension(
    const std::string &path,
    const std::vector<std::string> &extensions);

[[nodiscard]] bool isHttpUrl(const std::string &path);
[[nodiscard]] bool isValidDuration(float duration);

[[nodiscard]] inline AudioDurationResult resolveDurationFromDecoder(
    decoding::AudioDecoderBackend &decoder) {
  const float duration = decoder.getDurationInSeconds();
  if (!isValidDuration(duration)) {
    return Err("Audio duration metadata is unavailable");
  }
  return Ok(duration);
}

[[nodiscard]] AudioDurationResult probeDurationWithFilePath(const std::string &path);
[[nodiscard]] AudioDurationResult
probeDurationWithMemory(const void *data, size_t size, int sampleRate = 0);
[[nodiscard]] AudioDurationResult probeDurationWithUrl(
    const std::string &url,
    int sampleRate = 0,
    const std::map<std::string, std::string> &headers = {});

} // namespace audioapi::audiodecoding
