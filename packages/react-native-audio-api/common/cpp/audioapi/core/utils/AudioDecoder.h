#pragma once

#include <audioapi/core/types/AudioFormat.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Result.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace audioapi::audiodecoder {

using AudioBufferResult = Result<std::shared_ptr<AudioBuffer>, std::string>;

[[nodiscard]] AudioBufferResult decodeWithFilePath(const std::string &path, float sampleRate);
[[nodiscard]] AudioBufferResult
decodeWithMemoryBlock(const void *data, size_t size, float sampleRate);
[[nodiscard]] AudioBufferResult decodeWithPCMInBase64(
    const std::string &data,
    float inputSampleRate,
    int inputChannelCount,
    bool interleaved);

AudioFormat detectAudioFormat(const void *data, size_t size);

bool pathHasExtension(const std::string &path, const std::vector<std::string> &extensions);

bool needsFFmpeg(AudioFormat format) {
  return format == AudioFormat::MP4 || format == AudioFormat::M4A || format == AudioFormat::AAC;
}

bool needsFFmpegByPath(const std::string &path) {
  return pathHasExtension(path, {".mp4", ".m4a", ".aac"});
}

[[nodiscard]] float uint8ToFloat(uint8_t byte1, uint8_t byte2) {
  return static_cast<float>(static_cast<int16_t>((byte2 << CHAR_BIT) | byte1)) / INT16_MAX;
}

} // namespace audioapi::audiodecoder
