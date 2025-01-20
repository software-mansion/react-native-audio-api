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

float convertBytetoFloat(const uint8_t data) {
    return static_cast<float>(data) / 128.0f - 1.0f;
}

uint8_t convertFloatToByte(const float data) {
    float scaledValue = 128 * (data + 1);

    if (scaledValue < 0) {
        scaledValue = 0;
    }
    if (scaledValue > UINT8_MAX) {
        scaledValue = UINT8_MAX;
    }

    return static_cast<uint8_t>(scaledValue);
}
} // namespace audioapi::AudioUtils
