#pragma once

namespace audioapi {

struct StreamFormat {
  float sampleRate = 0.0f;
  int channelCount = 0;
  bool isInterleaved = true;
};

} // namespace audioapi
