#pragma once

#include <audioapi/libs/audio-stretch/stretch.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

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

  [[nodiscard]] static inline int16_t floatToInt16(float sample) {
    return static_cast<int16_t>(sample * 32768.0f);
  }
  [[nodiscard]] static inline float int16ToFloat(int16_t sample) {
    return static_cast<float>(sample) / 32768.0f;
  }
};

} // namespace audioapi
