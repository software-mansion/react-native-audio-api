#pragma once

#include <cstdint>

namespace audioapi {

enum class AudioContextLatencyHint : std::uint8_t { INTERACTIVE, BALANCED, PLAYBACK };

} // namespace audioapi
