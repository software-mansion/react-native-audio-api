#pragma once

#include <cstddef>

namespace audioapi::AudioUtils {
size_t timeToSampleFrame(double time, int sampleRate);

double sampleFrameToTime(int sampleFrame, int sampleRate);

float linearInterpolate(const float *source, size_t firstIndex, size_t secondIndex, float factor);

} // namespace audioapi::AudioUtils
