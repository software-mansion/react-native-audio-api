#pragma once

#include <cstddef>
#include <cstdint>
#include <cmath>

namespace audioapi::AudioUtils {

size_t timeToSampleFrame(double time, int sampleRate);
double sampleFrameToTime(int sampleFrame, int sampleRate);

float linearInterpolate(const float *source, size_t firstIndex, size_t secondIndex, float factor);

float convertBytetoFloat(uint8_t data);
uint8_t convertFloatToByte(float data);

float linearToDecibels(float value);
float decibelsToLinear(float value);
} // namespace audioapi::AudioUtils
