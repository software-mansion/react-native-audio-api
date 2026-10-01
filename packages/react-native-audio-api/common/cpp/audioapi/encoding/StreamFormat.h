#pragma once

#include <audioapi/utils/AudioLayout.h>

#include <cstddef>

namespace audioapi {

/// Layout of the float32 PCM a producer delivers.
struct StreamFormat {
  AudioLayout layout;
  /// The largest block delivered in one call; consumers size their buffers for it.
  size_t maxFramesPerBuffer = 0;
};

} // namespace audioapi
