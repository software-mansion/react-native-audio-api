#pragma once

#include <memory>

namespace audioapi {

class AudioBus;
class AudioBuffer;

class AudioStretcher {
 public:
  explicit AudioStretcher(float sampleRate) : sampleRate_(sampleRate) {}

  [[nodiscard]] std::shared_ptr<AudioBuffer> changePlaybackSpeed(
      AudioBuffer buffer,
      float playbackSpeed) const;

 private:
  float sampleRate_;

  [[nodiscard]] static inline int16_t floatToInt16(float sample) {
    return static_cast<int16_t>(sample * 32768.0f);
  }
  [[nodiscard]] static inline float int16ToFloat(int16_t sample) {
    return static_cast<float>(sample) / 32768.0f;
  }
};

} // namespace audioapi
