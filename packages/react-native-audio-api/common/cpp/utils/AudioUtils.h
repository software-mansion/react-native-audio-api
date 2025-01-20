#pragma once

#include <cstddef>
#include <cstdint>

namespace audioapi::AudioUtils {
size_t timeToSampleFrame(double time, int sampleRate);

double sampleFrameToTime(int sampleFrame, int sampleRate);

float linearInterpolate(const float *source, size_t firstIndex, size_t secondIndex, float factor);

float convertBytetoFloat(const uint8_t data);

uint8_t convertFloatToByte(const float data);

} // namespace audioapi::AudioUtils
