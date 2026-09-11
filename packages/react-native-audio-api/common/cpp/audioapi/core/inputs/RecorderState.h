#pragma once

#include <cstdint>

namespace audioapi {

/// @brief Lifecycle state of an AudioRecorder.
///
/// Mirrored by Kotlin's com.swmansion.audioapi.system.RecorderState, which maps the
/// values by ordinal — keep the two declarations in the same order.
enum class RecorderState : uint8_t { Idle = 0, Recording, Paused };

} // namespace audioapi
