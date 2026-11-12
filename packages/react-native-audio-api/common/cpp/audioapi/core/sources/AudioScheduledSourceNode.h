#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/types/NodeOptions.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

namespace audioapi {

class AudioScheduledSourceNode : public AudioNode {
 public:
  // UNSCHEDULED: The node is not scheduled to play.
  // SCHEDULED: The node is scheduled to play at a specific time.
  // PLAYING: The node is currently playing.
  // PAUSED: The node renders silence and keeps its position until resumed. Its stop time
  //         still applies: a node that is still paused at that time finishes without playing again.
  // STOP_SCHEDULED: The node is scheduled to stop at a specific time, but is still playing.
  // FINISHED: The node has finished playing.
  enum class PlaybackState : std::uint8_t {
    UNSCHEDULED,
    SCHEDULED,
    PLAYING,
    PAUSED,
    STOP_SCHEDULED,
    FINISHED
  };
  explicit AudioScheduledSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioScheduledSourceNodeOptions &options = AudioScheduledSourceNodeOptions());

  virtual void start(double when);
  virtual void stop(double when);

  /// @brief Suspends a scheduled or playing node; any other state is left alone.
  /// @note Audio Thread only
  void pausePlayback();

  /// @brief Schedules a paused node to continue at `when`, keeping its stop time.
  /// @note Audio Thread only
  void resumePlayback(double when);

  /// @note Audio Thread only
  bool isUnscheduled() const;

  /// @note Audio Thread only
  bool isScheduled() const;

  /// @note Audio Thread only
  bool isPlaying() const;

  /// @note Audio Thread only
  bool isPaused() const;

  /// @note Audio Thread only
  bool isFinished() const;

  /// @note Audio Thread only
  bool isStopScheduled() const;

  /// @brief Whether the node has finished for good. Unlike playbackState_, safe
  /// to read from any thread; set before `ended` is dispatched.
  [[nodiscard]] bool hasEnded() const;

  void disable() override;

  /// @brief Finishes the node (one-shot `ended`, then disable) once its stop time lies
  /// entirely before the quantum that is about to render, mirroring the "first silent
  /// quantum" branch of the render path. A source nobody pulls never renders, so without
  /// this a stopped-then-disconnected source would stay PLAYING, never fire `ended` and
  /// never satisfy canBeDestructed().
  /// @note Audio Thread only
  void advanceWithoutRender(std::size_t currentSampleFrame) override;

  void assignOnEndedCallbackId(uint64_t callbackId);

  bool canBeDestructed() const override;

  [[nodiscard]] std::optional<size_t> getOutputChannelNumber() const override;

 protected:
  double startTime_;
  double stopTime_;

  PlaybackState playbackState_;

  /// @note Audio Thread only
  void markEnded();

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
  std::atomic<bool> hasEnded_{false};

  void finishIfStopFrameReached(size_t stopFrame, size_t lastFrame);
};

} // namespace audioapi
