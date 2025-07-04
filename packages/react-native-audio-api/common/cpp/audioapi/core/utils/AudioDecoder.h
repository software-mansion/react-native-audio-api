#pragma once

#include <audioapi/libs/audio-stretch/stretch.h>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class AudioBus;

class AudioDecoder {
 public:
  explicit AudioDecoder(float sampleRate): sampleRate_(sampleRate) {}
  ~AudioDecoder() {
    stretch_deinit(stretcher_);
  }

  [[nodiscard]] std::shared_ptr<AudioBus> decodeWithFilePath(const std::string &path) const;
  [[nodiscard]] std::shared_ptr<AudioBus> decodeWithMemoryBlock(const void *data, size_t size) const;
  [[nodiscard]] std::shared_ptr<AudioBus> decodeWithPCMInBase64(const std::string &data, float playbackSpeed) const;

 private:
  float sampleRate_;
  int numChannels_ = 2;
  StretchHandle stretcher_ =
          stretch_init(static_cast<int>(sampleRate_ / 333.0f), static_cast<int>(sampleRate_ / 55.0f), 1,  0x1);

  void changePlaybackSpeedIfNeeded(std::vector<int16_t> &buffer, size_t framesDecoded, float playbackSpeed) const {
      if (playbackSpeed == 1.0f) {
          return;
      }

      int maxOutputFrames = stretch_output_capacity(stretcher_, static_cast<int>(framesDecoded), 1 / playbackSpeed);
      std::vector<int16_t> stretchedBuffer(maxOutputFrames);

      int outputFrames = stretch_samples(
              stretcher_,
              buffer.data(),
              static_cast<int>(framesDecoded),
              stretchedBuffer.data(),
              1 / playbackSpeed
      );

      outputFrames += stretch_flush(stretcher_, stretchedBuffer.data() + (outputFrames));
      stretchedBuffer.resize(outputFrames);

      buffer = stretchedBuffer;

      stretch_reset(stretcher_);
  }

  [[nodiscard]] static inline int16_t floatToInt16(float sample) {
    return static_cast<int16_t>(sample * 32768.0f);
  }

  [[nodiscard]] static inline float int16ToFloat(int16_t sample) {
    return static_cast<float>(sample) / 32768.0f;
  }
};

} // namespace audioapi
