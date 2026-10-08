#pragma once

#include <functional>

namespace audioapi {

/// @brief Runs closures off the calling thread. Implementations decide how many threads run
/// them and whether scheduling order is preserved.
/// @note Single producer: every implementation sits on SPSC channels, so schedule from one
/// thread only (the JS thread, or the thread that owns the executor).
class ITaskExecutor {
 public:
  virtual ~ITaskExecutor() = default;

  /// @brief Queues @p task. May block when the executor's queue is full.
  virtual void schedule(std::function<void()> &&task) = 0;
};

} // namespace audioapi
