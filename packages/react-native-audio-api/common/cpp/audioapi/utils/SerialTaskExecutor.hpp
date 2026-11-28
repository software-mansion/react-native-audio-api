#pragma once

#include <audioapi/utils/ITaskExecutor.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/TaskOffloader.hpp>

#include <cstddef>
#include <functional>
#include <utility>

namespace audioapi {

/// @brief One worker thread fed by an SPSC queue: tasks run strictly in scheduling order.
/// Use it where later calls must observe the effect of earlier ones, e.g. a recorder's
/// start / stop / pause lane. The destructor drains queued tasks before joining the worker.
class SerialTaskExecutor : public ITaskExecutor {
  using Offloader = task_offloader::TaskOffloader<
      std::function<void()>,
      channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      channels::spsc::WaitStrategy::ATOMIC_WAIT>;

 public:
  static constexpr size_t DEFAULT_CAPACITY = 32;

  explicit SerialTaskExecutor(size_t capacity = DEFAULT_CAPACITY)
      : offloader_(capacity, [](std::function<void()> &&task) { task(); }) {}
  DELETE_COPY_AND_MOVE(SerialTaskExecutor);
  ~SerialTaskExecutor() override = default;

  void schedule(std::function<void()> &&task) override {
    offloader_.send(std::move(task));
  }

 private:
  Offloader offloader_;
};

} // namespace audioapi
