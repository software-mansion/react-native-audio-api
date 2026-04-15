#pragma once

#include <cmath>
#include <limits>
#include <new>
#include <numbers>

// https://webaudio.github.io/web-audio-api/

namespace audioapi {
// audio
inline constexpr int RENDER_QUANTUM_SIZE = 128;
inline constexpr size_t MAX_FFT_SIZE = 32768;
inline constexpr int MAX_CHANNEL_COUNT = 32;
inline constexpr float DEFAULT_SAMPLE_RATE = 44100.0f;
inline constexpr int OCTAVE_RANGE = 1200;
inline constexpr int SEMITONES_PER_OCTAVE = 12;
inline constexpr int BIQUAD_GAIN_DB_FACTOR = 40;
inline constexpr float CENTS_TO_RATIO = 1.0f / 1200.0f;

// stretcher
inline constexpr float UPPER_FREQUENCY_LIMIT_DETECTION = 333.0f;
inline constexpr float LOWER_FREQUENCY_LIMIT_DETECTION = 55.0f;

// general
inline constexpr float MOST_POSITIVE_SINGLE_FLOAT =
    static_cast<float>(std::numeric_limits<float>::max());
inline constexpr float MOST_NEGATIVE_SINGLE_FLOAT =
    static_cast<float>(std::numeric_limits<float>::lowest());
inline const float LOG2_MOST_POSITIVE_SINGLE_FLOAT = std::log2(MOST_POSITIVE_SINGLE_FLOAT);
inline const float LOG10_MOST_POSITIVE_SINGLE_FLOAT = std::log10(MOST_POSITIVE_SINGLE_FLOAT);
inline constexpr float PI = std::numbers::pi_v<float>;

// buffer sizes
inline constexpr size_t PROMISE_VENDOR_THREAD_POOL_WORKER_COUNT = 4;
inline constexpr size_t PROMISE_VENDOR_THREAD_POOL_LOAD_BALANCER_QUEUE_SIZE = 32;
inline constexpr size_t PROMISE_VENDOR_THREAD_POOL_WORKER_QUEUE_SIZE = 32;

// Cache line size
#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

// audio param
inline constexpr size_t AUDIO_PARAM_MAX_QUEUED_EVENTS = 128;
} // namespace audioapi
