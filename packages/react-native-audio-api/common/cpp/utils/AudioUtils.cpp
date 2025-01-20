#include "AudioUtils.h"

namespace audioapi::AudioUtils {
size_t timeToSampleFrame(double time, int sampleRate) {
  return static_cast<size_t>(time * sampleRate);
}

double sampleFrameToTime(int sampleFrame, int sampleRate) {
  return static_cast<double>(sampleFrame) / sampleRate;
}

float linearInterpolate(
    const float *source,
    size_t firstIndex,
    size_t secondIndex,
    float factor) {
  if (firstIndex == secondIndex && firstIndex >= 1) {
    return source[firstIndex] +
        factor * (source[firstIndex] - source[firstIndex - 1]);
  }

  return source[firstIndex] +
      factor * (source[secondIndex] - source[firstIndex]);
}

float linearToDecibels(float value) {
    return 20 * log10f(value);
}

float decibelsToLinear(float value) {
    return powf(10, value / 20);
}
} // namespace audioapi::AudioUtils
