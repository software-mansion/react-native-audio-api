#pragma once

#include <audioapi/utils/SpscChannel.hpp>
#include <cassert>
#include <concepts>
#include <utility>

using namespace audioapi::channels::spsc;

namespace audioapi::task_offloader {

/// @brief A utility class to offload task to a separate thread using a SPSC channel.
/// @tparam T The type of data to be sent through the channel. Must be default_initializable.
/// @tparam Strategy The overflow strategy for the SPSC channel.
/// @tparam Wait The wait strategy for the SPSC channel.
template <std::default_initializable T, OverflowStrategy Strategy, WaitStrategy Wait>
class TaskOffloader {
 public:
  template <typename Func>
  explicit TaskOffloader(size_t capacity, Func &&task) : shouldRun_(true) {
    auto [sender, receiver] = channels::spsc::channel<T, Strategy, Wait>(capacity);
    sender_ = std::move(sender);
    receiver_ = std::move(receiver);

    workerThread_ = std::thread([this, task = std::forward<Func>(task)]() {
      while (shouldRun_.load(std::memory_order_acquire)) {
        auto data = receiver_.receive();
        if (shouldRun_.load(std::memory_order_acquire)) {
          task(std::move(data));
        }
      }
    });
  }

  // delete other functions
  TaskOffloader(const TaskOffloader &) = delete;
  TaskOffloader &operator=(const TaskOffloader &) = delete;
  TaskOffloader(TaskOffloader &&other) = delete;
  TaskOffloader &operator=(TaskOffloader &&other) = delete;

  ~TaskOffloader() {
    shouldRun_.store(false, std::memory_order_release);
    sender_.send(T{}); // send a dummy message to unblock the receiver
    if (workerThread_.joinable()) {
      workerThread_.join();
    }
  }

  /// @brief Get the const pointer to mutable sender of the SPSC channel
  auto *const getSender() {
    return &sender_;
  }

 private:
  Receiver<T, Strategy, Wait> receiver_;
  Sender<T, Strategy, Wait> sender_;
  std::thread workerThread_;
  std::atomic<bool> shouldRun_;
  bool taskOffloaded_;
};

} // namespace audioapi::task_offloader
