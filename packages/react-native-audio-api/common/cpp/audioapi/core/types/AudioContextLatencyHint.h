#pragma once

#include <audioapi/core/utils/Constants.h>

#include <cstdint>
#include <optional>

namespace audioapi {

enum class AudioContextLatencyHint : std::uint8_t { INTERACTIVE, BALANCED, PLAYBACK };

/// Output buffer this hint asks for, in frames of the output stream's own rate — not the
/// context's, which may differ. 0 requests nothing.
inline int preferredIOBufferFramesFor(std::optional<AudioContextLatencyHint> latencyHint) {
  if (!latencyHint.has_value()) {
    return 0;
  }

  switch (*latencyHint) {
    case AudioContextLatencyHint::INTERACTIVE:
      return RENDER_QUANTUM_SIZE;
    case AudioContextLatencyHint::BALANCED:
      return 4 * RENDER_QUANTUM_SIZE;
    case AudioContextLatencyHint::PLAYBACK:
      return 0;
  }

  return 0;
}

} // namespace audioapi
