#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#include "AudioNode.h"

namespace audioapi {

class AudioScheduledSourceNode : public AudioNode {
 public:
  enum class PlaybackState { UNSCHEDULED, SCHEDULED, PLAYING, FINISHED };
  explicit AudioScheduledSourceNode(BaseAudioContext *context);

  void start(double time);
  void stop(double time);

  bool isUnscheduled();
  bool isScheduled();
  bool isPlaying();
  bool isFinished();

 protected:
  std::atomic<PlaybackState> playbackState_;
  void updatePlaybackInfo(
      AudioBus *processingBus,
      int framesToProcess,
      size_t &startOffset,
      size_t &nonSilentFramesToProcess);

 private:
  double startTime_;
  double stopTime_;
};

} // namespace audioapi
