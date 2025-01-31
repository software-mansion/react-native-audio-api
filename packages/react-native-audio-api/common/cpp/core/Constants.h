#pragma once

#include <cmath>
#include <limits>

// https://webaudio.github.io/web-audio-api/

namespace audioapi {
// context
constexpr int DEFAULT_SAMPLE_RATE = 48000;
constexpr int RENDER_QUANTUM_SIZE = 128;
constexpr int CHANNEL_COUNT = 2;

// general
constexpr float MOST_POSITIVE_SINGLE_FLOAT = static_cast<float>(std::numeric_limits<float>::max());
constexpr float MOST_NEGATIVE_SINGLE_FLOAT = static_cast<float>(std::numeric_limits<float>::lowest());

constexpr float NYQUIST_FREQUENCY = DEFAULT_SAMPLE_RATE / 2.0;
constexpr float MAX_GAIN = MOST_POSITIVE_SINGLE_FLOAT;
constexpr float MAX_PAN = 1.0;
constexpr float MAX_FILTER_Q = MOST_POSITIVE_SINGLE_FLOAT;
constexpr float MAX_FILTER_FREQUENCY = NYQUIST_FREQUENCY;
constexpr float MIN_FILTER_FREQUENCY = 0.0;
static float MAX_FILTER_GAIN = 40 * std::log10(MOST_POSITIVE_SINGLE_FLOAT);
constexpr float MIN_FILTER_GAIN = -MAX_GAIN;

//detune
static float MAX_DETUNE = 1200 * std::log2(MOST_POSITIVE_SINGLE_FLOAT);
static float MIN_DETUNE = -MAX_DETUNE;

// analyser node
constexpr size_t MAX_FFT_SIZE = 32768;
constexpr size_t MIN_FFT_SIZE = 32;
constexpr size_t DEFAULT_FFT_SIZE = 2048;
constexpr float DEFAULT_MAX_DECIBELS = -30;
constexpr float DEFAULT_MIN_DECIBELS = -100;
const float DEFAULT_SMOOTHING_TIME_CONSTANT = 0.8;
} // namespace audioapi
