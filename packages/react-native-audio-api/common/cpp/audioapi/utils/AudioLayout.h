#pragma once

namespace audioapi {

/// Sample rate and channel count of a stream of audio.
struct AudioLayout {
  float sampleRate = 0.0F;
  int channelCount = 0;

  bool operator==(const AudioLayout &) const = default;
};

} // namespace audioapi
