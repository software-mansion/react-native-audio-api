#pragma once

#include <cmath>
#include <limits>

// https://webaudio.github.io/web-audio-api/

namespace audioapi {
constexpr int SAMPLE_RATE = 44100;
constexpr int BUFFER_SIZE = 128;
constexpr float MOST_POSITIVE_SINGLE_FLOAT =
    static_cast<float>(std::numeric_limits<float>::max());
constexpr float NYQUIST_FREQUENCY = SAMPLE_RATE / 2.0;
static float MAX_DETUNE = 1200 * std::log2(MOST_POSITIVE_SINGLE_FLOAT);
constexpr float MAX_GAIN = MOST_POSITIVE_SINGLE_FLOAT;
constexpr float MAX_PAN = 1.0;
constexpr float MAX_FILTER_Q = MOST_POSITIVE_SINGLE_FLOAT;
constexpr float MAX_FILTER_FREQUENCY = NYQUIST_FREQUENCY;
constexpr float MIN_FILTER_FREQUENCY = 0.0;
static float MAX_FILTER_GAIN = 40 * std::log10(MOST_POSITIVE_SINGLE_FLOAT);
constexpr float MIN_FILTER_GAIN = -MAX_GAIN;
} // namespace audioapi
