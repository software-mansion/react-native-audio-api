#pragma once

#include <audioapi/utils/SpscChannel.hpp>
#include <cassert>
#include <concepts>
#include <utility>

template <typename T>
concept DefaultConstructible = std::default_initializable<T>;

using namespace audioapi::channels::spsc;

namespace audioapi::task_offloader {

/// @brief A utility class to offload task to a separate thread using a SPSC channel.
/// @tparam T The type of data to be sent through the channel. Must be DefaultConstructible.
/// @tparam Strategy The overflow strategy for the SPSC channel.
/// @tparam Wait The wait strategy for the SPSC channel.
template <DefaultConstructible T, OverflowStrategy Strategy, WaitStrategy Wait>
class TaskOffloader {
 public:
  explicit TaskOffloader(Receiver<T, Strategy, Wait> &&receiver)
      : receiver_(std::move(receiver)), shouldRun_(false) {}

  // delete or other functions
  TaskOffloader(const TaskOffloader &) = delete;
  TaskOffloader &operator=(const TaskOffloader &) = delete;
  TaskOffloader(TaskOffloader &&other) = delete;
  TaskOffloader &operator=(TaskOffloader &&other) = delete;

  ~TaskOffloader() {
    auto wasStopCalled = !shouldRun_.load(std::memory_order_acquire);
    assert(
        wasStopCalled &&
        "TaskOffloader destructor called without stopping the offloader. Call stop() before destruction.");
  }

  /// @brief Offloads the given task to a separate thread.
  /// @param task The task to be offloaded. It should be a callable that takes a T as parameter.
  template <typename Func>
  void offloadTask(Func &&task) {
    shouldRun_.store(true, std::memory_order_release);
    workerThread_ = std::thread([this, task = std::forward<Func>(task)]() {
      while (shouldRun_.load(std::memory_order_acquire)) {
        auto data = receiver_.receive();
        if (shouldRun_.load(std::memory_order_acquire)) {
          task(std::move(data));
        }
      }
    });
  }

  /// @brief Stops the offloading thread and joins it.
  /// @param sender The sender associated with the receiver to send a dummy message to unblock the receiver.
  void stop(Sender<T, Strategy, Wait> &sender) {
    shouldRun_.store(false, std::memory_order_release);
    sender.send(T{}); // Send a dummy message to unblock the receiver
    if (workerThread_.joinable()) {
      workerThread_.join();
    }
  }

 private:
  Receiver<T, Strategy, Wait> receiver_;
  std::thread workerThread_;
  std::atomic<bool> shouldRun_;
};

} // namespace audioapi::task_offloader
