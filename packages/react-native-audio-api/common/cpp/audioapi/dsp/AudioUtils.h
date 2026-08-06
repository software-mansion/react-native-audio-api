#pragma once

#include <cmath>
#include <cstddef>
#include <span>

namespace audioapi::dsp {

[[nodiscard]] inline size_t timeToSampleFrame(double time, float sampleRate) {
  return static_cast<size_t>(time * sampleRate);
}

[[nodiscard]] inline double sampleFrameToTime(int sampleFrame, float sampleRate) {
  return static_cast<double>(sampleFrame) / sampleRate;
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables) -- function, not variable
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
  constexpr float kDecibelsLinearFactor = 20.0f;
  return kDecibelsLinearFactor * log10f(value);
}

[[nodiscard]] inline float decibelsToLinear(float value) {
  constexpr float kDecibelsDenominator = 20.0f;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
  return static_cast<float>(pow(10, value / kDecibelsDenominator));
}

} // namespace audioapi::dsp
