#pragma once

#include <audioapi/core/types/AudioFormat.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class AudioBuffer;

static constexpr int CHUNK_SIZE = 4096;

class AudioDecoder {
 public:
  AudioDecoder() = delete;

  [[nodiscard]] static std::shared_ptr<AudioBuffer> decodeWithFilePath(const std::string &path, float sampleRate);
  [[nodiscard]] static std::shared_ptr<AudioBuffer>
  decodeWithMemoryBlock(const void *data, size_t size, float sampleRate);
  [[nodiscard]] static std::shared_ptr<AudioBuffer>
  decodeWithPCMInBase64(const std::string &data, float inputSampleRate, int inputChannelCount, bool interleaved);

 private:
  static std::vector<float> readAllPcmFrames(ma_decoder &decoder, int outputChannels);
  static std::shared_ptr<AudioBuffer>
  makeAudioBufferFromFloatBuffer(const std::vector<float> &buffer, float outputSampleRate, int outputChannels);

  static AudioFormat detectAudioFormat(const void *data, size_t size) {
    if (size < 12)
      return AudioFormat::UNKNOWN;
    const auto *bytes = static_cast<const unsigned char *>(data);

    // WAV/RIFF
    if (std::memcmp(bytes, "RIFF", 4) == 0 && std::memcmp(bytes + 8, "WAVE", 4) == 0)
      return AudioFormat::WAV;

    // OGG
    if (std::memcmp(bytes, "OggS", 4) == 0)
      return AudioFormat::OGG;

    // FLAC
    if (std::memcmp(bytes, "fLaC", 4) == 0)
      return AudioFormat::FLAC;

    // AAC starts with 0xFF 0xF1 or 0xFF 0xF9
    if (bytes[0] == 0xFF && (bytes[1] & 0xF6) == 0xF0)
      return AudioFormat::AAC;

    // MP3: "ID3" or 11-bit frame sync (0xFF 0xE0)
    if (std::memcmp(bytes, "ID3", 3) == 0)
      return AudioFormat::MP3;
    if (bytes[0] == 0xFF && (bytes[1] & 0xE0) == 0xE0)
      return AudioFormat::MP3;

    if (std::memcmp(bytes + 4, "ftyp", 4) == 0) {
      if (std::memcmp(bytes + 8, "M4A ", 4) == 0)
        return AudioFormat::M4A;
      else if (std::memcmp(bytes + 8, "qt  ", 4) == 0)
        return AudioFormat::MOV;
      return AudioFormat::MP4;
    }
    return AudioFormat::UNKNOWN;
  }

  static inline bool pathHasExtension(const std::string &path, const std::vector<std::string> &extensions) {
    std::string pathLower = path;
    std::transform(pathLower.begin(), pathLower.end(), pathLower.begin(), ::tolower);
    for (const auto &ext : extensions) {
      if (pathLower.ends_with(ext))
        return true;
    }
    return false;
  }

  [[nodiscard]] static inline int16_t floatToInt16(float sample) {
    return static_cast<int16_t>(sample * INT16_MAX);
  }
  [[nodiscard]] static inline float int16ToFloat(int16_t sample) {
    return static_cast<float>(sample) / INT16_MAX;
  }
  [[nodiscard]] static inline float uint8ToFloat(uint8_t byte1, uint8_t byte2) {
    return static_cast<float>(static_cast<int16_t>((byte2 << 8) | byte1)) / INT16_MAX;
  }
};

} // namespace audioapi
