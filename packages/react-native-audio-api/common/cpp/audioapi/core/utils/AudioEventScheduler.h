#pragma once

#include <audioapi/utils/CrossThreadEventScheduler.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace audioapi {

class BaseAudioContext;

/// @brief A scheduler for audio events.
///
/// It is a wrapper around CrossThreadEventScheduler specialized for audio events.
/// It is designed to be managed by BaseAudioContext.
/// @note If context is running events are scheduled to be executed on audio thread.
/// @note If context is not running events are executed immediately on scheduling thread.
class AudioEventScheduler {
 public:
  /// @brief Constructs an AudioEventScheduler with the given context and capacity.
  /// @param context The audio context associated with this scheduler.
  /// @param capacity The maximum number of events that can be scheduled. Default is 1024.
  explicit AudioEventScheduler(
      const std::shared_ptr<BaseAudioContext> &context,
      size_t capacity = 1024);
  ~AudioEventScheduler() = default;

  /// @brief Schedules an event to be processed.
  bool scheduleEvent(std::function<void(BaseAudioContext &)> &&event) noexcept;

  /// @brief Processes all scheduled events.
  void processAllEvents();

 private:
  std::shared_ptr<BaseAudioContext> context_;
  CrossThreadEventScheduler<BaseAudioContext> eventScheduler_;
};
} // namespace audioapi
