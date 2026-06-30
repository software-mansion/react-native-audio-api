#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/types/NodeOptions.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
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
  enum class PlaybackState : std::uint8_t {
    UNSCHEDULED,
    SCHEDULED,
    PLAYING,
    STOP_SCHEDULED,
    FINISHED
  };
  explicit AudioScheduledSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioScheduledSourceNodeOptions &options = AudioScheduledSourceNodeOptions());

  virtual void start(double when);
  virtual void stop(double when);

  /// @note Audio Thread only
  bool isUnscheduled() const;

  /// @note Audio Thread only
  bool isScheduled() const;

  /// @note Audio Thread only
  bool isPlaying() const;

  /// @note Audio Thread only
  bool isFinished() const;

  /// @note Audio Thread only
  bool isStopScheduled() const;

  void disable() override;

  void assignOnEndedCallbackId(uint64_t callbackId);

  bool canBeDestructed() const override;

 protected:
  double startTime_;
  double stopTime_;

  PlaybackState playbackState_;

  void updatePlaybackInfo(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess,
      size_t &startOffset,
      size_t &nonSilentFramesToProcess,
      float sampleRate,
      size_t currentSampleFrame);

  void handleStopScheduled();

 private:
  EventCaller<AudioEvent::ENDED> onEndedEvent_;
};

} // namespace audioapi
