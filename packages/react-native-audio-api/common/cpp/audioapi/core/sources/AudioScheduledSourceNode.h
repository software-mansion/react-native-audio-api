#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/types/NodeOptions.h>

#include <cassert>
#include <cstddef>
#include <memory>

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
  explicit AudioScheduledSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioScheduledSourceNodeOptions &options = AudioScheduledSourceNodeOptions());

  virtual void start(double when);
  virtual void stop(double when);

  /// @note Audio Thread only
  bool isUnscheduled();

  /// @note Audio Thread only
  bool isScheduled();

  /// @note Audio Thread only
  bool isPlaying();

  /// @note Audio Thread only
  bool isFinished();

  /// @note Audio Thread only
  bool isStopScheduled();

  /// @note Audio Thread only
  void setOnEndedCallbackId(uint64_t callbackId);

  void disable() override;

  void unregisterOnEndedCallback(uint64_t callbackId);

 protected:
  double startTime_;
  double stopTime_;

  PlaybackState playbackState_;

  uint64_t onEndedCallbackId_ = 0;
  const std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;

  void updatePlaybackInfo(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess,
      size_t &startOffset,
      size_t &nonSilentFramesToProcess,
      float sampleRate,
      size_t currentSampleFrame);

  void handleStopScheduled();
};

} // namespace audioapi
