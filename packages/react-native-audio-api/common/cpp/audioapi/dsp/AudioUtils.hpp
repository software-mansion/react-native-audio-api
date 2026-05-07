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

// Hermite 4-point interpolation for smooth looping.
// Unlike linear interpolation, Hermite matches both value AND slope at
// the interpolation point, eliminating audible clicks at loop boundaries
// when playbackRate != 1.0.
[[nodiscard]] inline float hermiteInterpolate(
    std::span<const float> source,
    size_t idx0,
    size_t idx1,
    size_t idx2,
    size_t idx3,
    float t) {
  float y0 = source[idx0], y1 = source[idx1], y2 = source[idx2], y3 = source[idx3];
  float c0 = y1;
  float c1 = 0.5f * (y2 - y0);
  float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
  float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
  return ((c3 * t + c2) * t + c1) * t + c0;
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
