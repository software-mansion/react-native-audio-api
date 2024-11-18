#pragma once

#include <atomic>

#include "AudioNode.h"

namespace audioapi {

class AudioScheduledSourceNode : public AudioNode {
 public:
  explicit AudioScheduledSourceNode(BaseAudioContext *context);

  void start(double time);
  void stop(double time);

 protected:
  std::atomic<bool> isPlaying_;
  std::atomic<double> nextChangeTime_ { -1.0 };

  void handlePlayback(double time, int framesToProcess);
};

} // namespace audioapi
