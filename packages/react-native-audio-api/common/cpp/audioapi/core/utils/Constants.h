#pragma once

#include <cmath>
#include <limits>
#include <numbers>

// https://webaudio.github.io/web-audio-api/

namespace audioapi {
// audio
static constexpr int RENDER_QUANTUM_SIZE = 128;
static constexpr size_t MAX_FFT_SIZE = 32768;
static constexpr int MAX_CHANNEL_COUNT = 32;

// stretcher
static constexpr float UPPER_FREQUENCY_LIMIT_DETECTION = 333.0f;
static constexpr float LOWER_FREQUENCY_LIMIT_DETECTION = 55.0f;

// general
static constexpr float MOST_POSITIVE_SINGLE_FLOAT =
    static_cast<float>(std::numeric_limits<float>::max());
static constexpr float MOST_NEGATIVE_SINGLE_FLOAT =
    static_cast<float>(std::numeric_limits<float>::lowest());
static float LOG2_MOST_POSITIVE_SINGLE_FLOAT = std::log2(MOST_POSITIVE_SINGLE_FLOAT);
static float LOG10_MOST_POSITIVE_SINGLE_FLOAT = std::log10(MOST_POSITIVE_SINGLE_FLOAT);
static constexpr float PI = std::numbers::pi_v<float>;

// buffer sizes
static constexpr size_t PROMISE_VENDOR_THREAD_POOL_WORKER_COUNT = 4;
static constexpr size_t PROMISE_VENDOR_THREAD_POOL_LOAD_BALANCER_QUEUE_SIZE = 32;
static constexpr size_t PROMISE_VENDOR_THREAD_POOL_WORKER_QUEUE_SIZE = 32;
} // namespace audioapi
