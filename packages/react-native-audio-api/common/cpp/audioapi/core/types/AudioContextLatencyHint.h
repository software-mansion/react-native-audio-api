#pragma once

#include <cstdint>

namespace audioapi {

/// Web Audio's AudioContextOptions.latencyHint categories: what the context should
/// optimize its output stream for. Platform backends map these to their own stream
/// configuration; INTERACTIVE preserves the pre-hint behaviour and is the default.
enum class AudioContextLatencyHint : std::uint8_t { INTERACTIVE, BALANCED, PLAYBACK };

} // namespace audioapi
