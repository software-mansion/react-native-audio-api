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

class AudioEventHandlerRegistry;

class AudioScheduledSourceNode : public AudioNode {
 public:
  // UNSCHEDULED: The node is not scheduled to play.
  // SCHEDULED: The node is scheduled to play at a specific time.
  // PLAYING: The node is currently playing.
  // FINISHED: The node has finished playing.
  // STOP_SCHEDULED: The node is scheduled to stop at a specific time, but is still playing.
  enum class PlaybackState { UNSCHEDULED, SCHEDULED, PLAYING, FINISHED, STOP_SCHEDULED };
  explicit AudioScheduledSourceNode(BaseAudioContext *context);

  void start(double when);
  void stop(double when);

  bool isUnscheduled();
  bool isScheduled();
  bool isPlaying();
  bool isFinished();
  bool isStopScheduled();

  void setOnEndedCallbackId(uint64_t callbackId);

  virtual double getStopTime() const = 0;
  void disable() override;

 protected:
  double startTime_;
  double stopTime_;

  PlaybackState playbackState_;

  void updatePlaybackInfo(
      const std::shared_ptr<AudioBus>& processingBus,
      int framesToProcess,
      size_t &startOffset,
      size_t &nonSilentFramesToProcess);

  void handleStopScheduled();

  uint64_t onEndedCallbackId_ = 0;
};

} // namespace audioapi
