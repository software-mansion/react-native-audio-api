#pragma once

#include <audioapi/utils/Macros.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <concepts>
#include <optional>
#include <thread>
#include <utility>

using namespace audioapi::channels::spsc;

namespace audioapi::task_offloader {

/// @brief A utility class to offload task to a separate thread using a SPSC channel.
/// @tparam T The type of data to be sent through the channel.
/// @tparam Strategy The overflow strategy for the SPSC channel.
/// @tparam Wait The wait strategy for the SPSC channel.
template <typename T, OverflowStrategy Strategy, WaitStrategy Wait>
class TaskOffloader {
  /// An empty message asks the worker thread to stop.
  using Message = std::optional<T>;

 public:
  template <std::invocable<T> Func>
  explicit TaskOffloader(size_t capacity, Func &&task) {
    auto [sender, receiver] = channels::spsc::channel<Message, Strategy, Wait>(capacity);
    sender_ = std::move(sender);
    receiver_ = std::move(receiver);

    workerThread_ = std::thread([this, task = std::forward<Func>(task)]() {
      while (Message message = receiver_.receive()) {
        task(std::move(*message));
      }
    });
  }

  // delete other functions
  DELETE_COPY_AND_MOVE(TaskOffloader);

  ~TaskOffloader() {
    shutdown();
  }

  /// @brief Hands @p value to the worker thread.
  /// @note One thread at a time: the channel has a single producer.
  void send(T &&value) {
    sender_.send(Message{std::move(value)});
  }

  /// @brief Drains any queued tasks, then stops the worker thread.
  void shutdown() {
    if (!workerThread_.joinable()) {
      return;
    }
    sender_.send(std::nullopt);
    workerThread_.join();
  }

 private:
  Receiver<Message, Strategy, Wait> receiver_;
  Sender<Message, Strategy, Wait> sender_;
  std::thread workerThread_;
};

} // namespace audioapi::task_offloader
