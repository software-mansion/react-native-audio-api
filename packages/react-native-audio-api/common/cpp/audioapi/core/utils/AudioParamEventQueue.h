#pragma once

#include <audioapi/core/types/ParamChangeEventType.h>
#include <audioapi/core/utils/ParamChangeEvent.h>
#include <audioapi/utils/RingBiDirectionalBuffer.hpp>

namespace audioapi {

/// @brief A queue for managing audio parameter change events.
/// @note The invariant of the queue is that its internal buffer always contains non-overlapping events.
class AudioParamEventQueue {
 public:
  /// @brief Constructor for AudioParamEventQueue.
  /// @note Capacity must be valid power of two.
  explicit AudioParamEventQueue();

  /// @brief Push a new event to the back of the queue.
  /// @note Handles connecting the start value of the new event to the end value of the last event in the queue.
  void pushBack(ParamChangeEvent &&event);

  /// @brief Pop the front event from the queue.
  /// @return The front event in the queue.
  bool popFront(ParamChangeEvent &event);

  /// @brief Cancel scheduled parameter changes at or after the given time.
  /// @param cancelTime The time at which to cancel scheduled changes.
  void cancelScheduledValues(double cancelTime);

  /// @brief Cancel scheduled parameter changes and hold the current value at the given time.
  /// @param cancelTime The time at which to cancel scheduled changes.
  void cancelAndHoldAtTime(double cancelTime, double &endTimeCache);

  /// @brief Get the first event in the queue.
  /// @return The first event in the queue.
  inline const ParamChangeEvent &front() const noexcept {
    return eventQueue_.peekFront();
  }

  /// @brief Get the last event in the queue.
  /// @return The last event in the queue.
  inline const ParamChangeEvent &back() const noexcept {
    return eventQueue_.peekBack();
  }

  /// @brief Check if the event queue is empty.
  /// @return True if the queue is empty, false otherwise.
  inline bool isEmpty() const noexcept {
    return eventQueue_.isEmpty();
  }

  /// @brief Check if the event queue is full.
  /// @return True if the queue is full, false otherwise.
  inline bool isFull() const noexcept {
    return eventQueue_.isFull();
  }

 private:
  /// @brief The queue of parameter change events.
  /// @note INVARIANT it always holds non-overlapping events sorted by start time.
  RingBiDirectionalBuffer<ParamChangeEvent, 32> eventQueue_;
};

} // namespace audioapi
