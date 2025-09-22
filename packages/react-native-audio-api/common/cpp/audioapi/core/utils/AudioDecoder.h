#pragma once

#include <audioapi/libs/audio-stretch/stretch.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

namespace audioapi {

enum class AudioFormat {
  UNKNOWN,
  WAV,
  OGG,
  FLAC,
  AAC,
  MP3,
  M4A,
  MP4,
  MOV
};

class AudioBus;

static constexpr int CHUNK_SIZE = 4096;

class AudioDecoder {
 public:
  explicit AudioDecoder(float sampleRate) : sampleRate_(sampleRate) {}

  [[nodiscard]] std::shared_ptr<AudioBus> decodeWithFilePath(
      const std::string &path) const;
  [[nodiscard]] std::shared_ptr<AudioBus> decodeWithMemoryBlock(
      const void *data,
      size_t size) const;
  [[nodiscard]] std::shared_ptr<AudioBus> decodeWithPCMInBase64(
      const std::string &data,
      float playbackSpeed) const;

 private:
  float sampleRate_;
  int numChannels_ = 2;

  static std::vector<int16_t> readAllPcmFrames(
      ma_decoder &decoder,
      int numChannels,
      ma_uint64 &outFramesRead);
  static std::shared_ptr<AudioBus> makeAudioBusFromInt16Buffer(
      const std::vector<int16_t> &buffer,
      int numChannels,
      float sampleRate);

  void changePlaybackSpeedIfNeeded(
      std::vector<int16_t> &buffer,
      size_t framesDecoded,
      int numChannels,
      float playbackSpeed) const {
    if (playbackSpeed == 1.0f) {
      return;
    }

    auto stretcher = stretch_init(
        static_cast<int>(sampleRate_ / 333.0f),
        static_cast<int>(sampleRate_ / 55.0f),
        numChannels,
        0x1);

    int maxOutputFrames = stretch_output_capacity(
        stretcher, static_cast<int>(framesDecoded), 1 / playbackSpeed);
    std::vector<int16_t> stretchedBuffer(maxOutputFrames);

    int outputFrames = stretch_samples(
        stretcher,
        buffer.data(),
        static_cast<int>(framesDecoded),
        stretchedBuffer.data(),
        1 / playbackSpeed);

    outputFrames +=
        stretch_flush(stretcher, stretchedBuffer.data() + (outputFrames));
    stretchedBuffer.resize(outputFrames);

    buffer = stretchedBuffer;

    stretch_deinit(stretcher);
  }

  static AudioFormat detectAudioFormat(const void* data, size_t size) {
    if (size < 12) return AudioFormat::UNKNOWN;
    const auto* bytes = static_cast<const unsigned char*>(data);

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
    for (const auto& ext : extensions) {
        if (pathLower.ends_with(ext))
            return true;
    }
    return false;
  }


  [[nodiscard]] static inline int16_t floatToInt16(float sample) {
    return static_cast<int16_t>(sample * 32768.0f);
  }
  [[nodiscard]] static inline float int16ToFloat(int16_t sample) {
    return static_cast<float>(sample) / 32768.0f;
  }
};

} // namespace audioapi
