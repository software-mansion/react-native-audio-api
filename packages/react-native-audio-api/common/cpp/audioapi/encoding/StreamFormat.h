#pragma once

#include <cstddef>

namespace audioapi {

/// Layout of the float32 PCM a producer delivers.
struct StreamFormat {
  float sampleRate = 0.0f;
  int channelCount = 0;
  /// The largest block delivered in one call; consumers size their buffers for it.
  size_t maxFramesPerBuffer = 0;
};

} // namespace audioapi
