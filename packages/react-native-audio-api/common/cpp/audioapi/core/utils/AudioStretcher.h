#pragma once

#include <memory>
#include <vector>

namespace audioapi {

class AudioBus;
class AudioBuffer;

class AudioStretcher {
 public:
  AudioStretcher() = delete;

  [[nodiscard]] static std::shared_ptr<AudioBuffer> changePlaybackSpeed(
      AudioBuffer buffer,
      float playbackSpeed);

 private:
  static std::vector<int16_t> castToInt16Buffer(AudioBuffer &buffer);

  [[nodiscard]] static inline int16_t floatToInt16(float sample) {
    return static_cast<int16_t>(sample * INT16_MAX);
  }
  [[nodiscard]] static inline float int16ToFloat(int16_t sample) {
    return static_cast<float>(sample) / INT16_MAX;
  }
};

} // namespace audioapi
