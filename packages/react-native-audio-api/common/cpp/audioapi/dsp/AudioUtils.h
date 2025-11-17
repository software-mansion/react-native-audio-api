#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace audioapi::dsp {

size_t timeToSampleFrame(double time, float sampleRate);
double sampleFrameToTime(int sampleFrame, float sampleRate);

float linearInterpolate(const float *source, size_t firstIndex, size_t secondIndex, float factor);

float linearToDecibels(float value);
float decibelsToLinear(float value);
} // namespace audioapi::dsp
