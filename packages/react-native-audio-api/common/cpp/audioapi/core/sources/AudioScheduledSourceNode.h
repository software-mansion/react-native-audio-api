#pragma once

#include <audioapi/core/AudioNode.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <thread>
#include <cstddef>
#include <cassert>
#include <utility>

namespace audioapi {

class AudioScheduledSourceNode : public AudioNode {
 public:
  enum class PlaybackState { UNSCHEDULED, SCHEDULED, PLAYING, FINISHED };
  explicit AudioScheduledSourceNode(BaseAudioContext *context);

  void start(double when);
  void stop(double when);

  bool isUnscheduled();
  bool isScheduled();
  bool isPlaying();
  bool isFinished();

  void setOnendedCallback(const std::function<void(double)> &onendedCallback);

  virtual double getStopTime() const = 0;

 protected:
  PlaybackState playbackState_;

  std::function<void(double)> onendedCallback_;

  void updatePlaybackInfo(
      const std::shared_ptr<AudioBus>& processingBus,
      int framesToProcess,
      size_t &startOffset,
      size_t &nonSilentFramesToProcess);

  void handleStopScheduled();

 private:
  double startTime_;
  double stopTime_;
};

} // namespace audioapi
