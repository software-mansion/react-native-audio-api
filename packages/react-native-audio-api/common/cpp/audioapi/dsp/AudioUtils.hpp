#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace audioapi::dsp {

[[nodiscard]] inline size_t timeToSampleFrame(double time, float sampleRate) {
  return static_cast<size_t>(time * sampleRate);
}

[[nodiscard]] inline double sampleFrameToTime(int sampleFrame, float sampleRate) {
  return static_cast<double>(sampleFrame) / sampleRate;
}

[[nodiscard]] inline float linearInterpolate(
    std::span<const float> source,
    size_t firstIndex,
    size_t secondIndex,
    float factor) {

  if (firstIndex == secondIndex && firstIndex >= 1) {
    return source[firstIndex] + factor * (source[firstIndex] - source[firstIndex - 1]);
  }

  return std::lerp(source[firstIndex], source[secondIndex], factor);
}

[[nodiscard]] inline float linearToDecibels(float value) {
  return 20.0f * log10f(value);
}

[[nodiscard]] inline float decibelsToLinear(float value) {
  return static_cast<float>(pow(10, value / 20));
}

} // namespace audioapi::dsp
