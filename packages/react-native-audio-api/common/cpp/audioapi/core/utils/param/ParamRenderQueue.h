#pragma once

#include <audioapi/core/utils/param/ParamQueueBase.hpp>
#include <audioapi/core/utils/param/RenderParamEvent.h>
#include <optional>

namespace audioapi {

/// @brief A queue for managing audio parameter change events on the audio render thread.
/// @note The invariant of the queue is that its internal buffer always contains non-overlapping events.
///
/// Event times are kept in two forms (see RenderParamEvent): push() snaps the
/// inherited times onto the sample-frame grid of @c sampleRate while preserving
/// the scheduled times in the raw fields, so effect boundaries land on
/// round(T * sampleRate) frames and interpolation still runs on real times.
class ParamRenderQueue : public ParamQueueBase<RenderParamEvent> {
 public:
  explicit ParamRenderQueue(float defaultValue, float sampleRate)
      : defaultValue_(defaultValue), sampleRate_(sampleRate) {}

  /// @brief Compute the value at a specific time based on the events in the queue.
  /// @param time The time at which to compute the value.
  /// @return The computed value at the given time, or std::nullopt if no events are active at that time.
  [[nodiscard]] std::optional<float> computeValueAtTime(double time);

  /// @brief Push a new event into the queue, resolving its startValue and startTime based on neighboring events.
  /// @param event The new event to add to the queue.
  /// @return True if the event was successfully added, false if the queue is full.
  bool push(RenderParamEvent &&event) override;

  /// @brief Cancel scheduled parameter changes at or after the given time
  /// (compared on the snapped time grid).
  void cancelScheduledValues(double cancelTime) override {
    ParamQueueBase::cancelScheduledValues(snapToSampleFrameTime(cancelTime));
  }

  /// @brief Cancel scheduled parameter changes and hold the current value at the given time.
  /// @param cancelTime The time at which to cancel scheduled changes.
  void cancelAndHoldAtTime(double cancelTime);

 private:
  float defaultValue_;
  float sampleRate_;

  /// @brief Snap a time to the exact time of its nearest sample frame
  /// (<tt>round(time * sampleRate) / sampleRate</tt>), the grid the render
  /// loop evaluates frames on.
  [[nodiscard]] double snapToSampleFrameTime(double time) const;

  /// @brief Resolve new event's startValue and startTime based on the previous event in the queue,
  /// and adjust neighboring events to maintain the invariant of non-overlapping events in the queue.
  /// @param event The new event to resolve around.
  void resolveEventValues(RenderParamEvent &event);

  /// @brief Compute the value of the previous event at a specific time.
  /// @param event The preceding event.
  /// @param time The time at which to get the value.
  /// @return The value of the event at the given time.
  static float getValueOfPreviousEventAt(const RenderParamEvent &event, double time);

  /// @brief The currently active event that has been popped from the queue but has not yet ended or been replaced.
  // This is needed to handle the case where there are no future events in the queue,
  // but we still need to compute values for the active event.
  std::optional<RenderParamEvent> currentEvent_ = std::nullopt;
};

} // namespace audioapi
