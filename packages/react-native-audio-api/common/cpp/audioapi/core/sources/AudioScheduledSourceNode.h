#pragma once

#include <audioapi/core/AudioNode.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <thread>
#include <utility>

namespace audioapi {

class IAudioEventHandlerRegistry;

class AudioScheduledSourceNode : public AudioNode {
 public:
  // UNSCHEDULED: The node is not scheduled to play.
  // SCHEDULED: The node is scheduled to play at a specific time.
  // PLAYING: The node is currently playing.
  // STOP_SCHEDULED: The node is scheduled to stop at a specific time, but is still playing.
  // FINISHED: The node has finished playing.
  enum class PlaybackState { UNSCHEDULED, SCHEDULED, PLAYING, STOP_SCHEDULED, FINISHED };
  explicit AudioScheduledSourceNode(std::shared_ptr<BaseAudioContext> context);

  virtual void start(double when);
  virtual void stop(double when);

  bool isUnscheduled();
  bool isScheduled();
  bool isPlaying();
  bool isFinished();
  bool isStopScheduled();

  void setOnEndedCallbackId(uint64_t callbackId);

  void disable() override;

 protected:
  double startTime_;
  double stopTime_;

  PlaybackState playbackState_;

  std::atomic<uint64_t> onEndedCallbackId_ = 0;
  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;

  void updatePlaybackInfo(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess,
      size_t &startOffset,
      size_t &nonSilentFramesToProcess,
      float sampleRate,
      size_t currentSampleFrame);

  void handleStopScheduled();
};

} // namespace audioapi
