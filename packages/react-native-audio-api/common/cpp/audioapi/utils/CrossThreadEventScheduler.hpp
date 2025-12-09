#pragma once

#include <audioapi/utils/SpscChannel.hpp>

#include <functional>
#include <type_traits>
#include <utility>

namespace audioapi {
using namespace channels::spsc;

/// @brief A scheduler for cross-thread events.
/// @tparam T The type of data to be processed by the events.
////
/// It lets to schedule an event that should be performed on a different thread.
///
/// - T1 - JS thread
///
/// - T2 - Audio thread
///
/// Audio thread is looping every render quantum (~128 frames).
/// We want to modify some parameter on the resource used by audio thread.
/// We can schedule function on JS-Thread and process it before processing frames.
/// In this setup no locking happens and modifications can be seen by Audio thread.
/// @note it is intended to be used for two threads one which schedules events and one which processes them
/// @note it is not safe to be copied across two threads use std::shared_ptr if you need to share data
template <typename T>
class CrossThreadEventScheduler {
 public:
  explicit CrossThreadEventScheduler(size_t capacity) {
    auto [sender, receiver] = channel<std::function<void(T &)>>(capacity);
    eventSender_ = std::move(sender);
    eventReceiver_ = std::move(receiver);
  }
  CrossThreadEventScheduler(const CrossThreadEventScheduler &) = delete;
  CrossThreadEventScheduler &operator=(const CrossThreadEventScheduler &) = delete;

  /// @brief Schedules an event to be processed on the audio thread.
  /// @param event The event to schedule.
  /// @return True if the event was successfully scheduled, false if the queue is full.
  bool scheduleEvent(std::function<void(T &)> &&event) noexcept(
      noexcept(eventSender_.try_send(std::move(event)))) {
    return eventSender_.try_send(std::move(event)) == ResponseStatus::SUCCESS;
  }

  /// @brief Processes all scheduled events.
  /// @param data The data to pass to each event.
  void processAllEvents(T &data) noexcept(
      noexcept(eventReceiver_.try_receive(std::declval<std::function<void(T &)> &>()))) {
    std::function<void(T &)> event;
    while (eventReceiver_.try_receive(event) == ResponseStatus::SUCCESS) {
      event(data);
    }
  }

 private:
  Sender<std::function<void(T &)>> eventSender_;
  Receiver<std::function<void(T &)>> eventReceiver_;
};

} // namespace audioapi
