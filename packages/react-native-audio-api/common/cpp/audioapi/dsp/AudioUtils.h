#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace audioapi::dsp {

/// Round to the nearest frame.
[[nodiscard]] inline size_t timeToSampleFrame(double time, float sampleRate) {
  return static_cast<size_t>(0.5 + time * sampleRate);
}

/// Fractional frames, with float noise near a whole frame snapped away.
[[nodiscard]] inline double timeToFractionalFrames(double time, float sampleRate) {
  constexpr double kWholeFrameTolerance = 1e-6;
  const double frames = time * sampleRate;
  const double wholeFrames = std::round(frames);
  return std::fabs(frames - wholeFrames) < kWholeFrameTolerance ? wholeFrames : frames;
}

[[nodiscard]] inline double sampleFrameToTime(int sampleFrame, float sampleRate) {
  return static_cast<double>(sampleFrame) / sampleRate;
}

/// Convert little-endian int16 sample bytes to float in [-1, 1].
[[nodiscard]] inline float uint8ToFloat(uint8_t byte1, uint8_t byte2) {
  return static_cast<float>(static_cast<int16_t>((byte2 << 8) | byte1)) /
      static_cast<float>(INT16_MAX);
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
