#pragma once

#include <audioapi/utils/Macros.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <cassert>
#include <concepts>
#include <limits>
#include <type_traits>
#include <utility>

using namespace audioapi::channels::spsc;

namespace audioapi::task_offloader {

namespace detail {

template <typename T, typename = void>
struct HasSlotMember : std::false_type {};

template <typename T>
struct HasSlotMember<T, std::void_t<decltype(std::declval<T>().slot)>> : std::true_type {};

template <typename T>
bool isShutdownMessage(const T &data) {
  if constexpr (HasSlotMember<T>::value) {
    return data.slot == std::numeric_limits<decltype(data.slot)>::max();
  } else {
    return data == T{};
  }
}

} // namespace detail

/// @brief A utility class to offload task to a separate thread using a SPSC channel.
/// @tparam T The type of data to be sent through the channel. Must be default_initializable.
/// @tparam Strategy The overflow strategy for the SPSC channel.
/// @tparam Wait The wait strategy for the SPSC channel.
template <std::default_initializable T, OverflowStrategy Strategy, WaitStrategy Wait>
class TaskOffloader {
 public:
  template <std::invocable<T> Func>
  explicit TaskOffloader(size_t capacity, Func &&task) {
    auto [sender, receiver] = channels::spsc::channel<T, Strategy, Wait>(capacity);
    sender_ = std::move(sender);
    receiver_ = std::move(receiver);

    workerThread_ = std::thread([this, task = std::forward<Func>(task)]() {
      while (true) {
        T data = receiver_.receive();
        if (detail::isShutdownMessage(data)) {
          T pending;
          while (receiver_.try_receive(pending) == ResponseStatus::SUCCESS) {
            if (!detail::isShutdownMessage(pending)) {
              task(std::move(pending));
            }
          }
          break;
        }
        task(std::move(data));
      }
    });
  }

  // delete other functions
  DELETE_COPY_AND_MOVE(TaskOffloader);

  ~TaskOffloader() {
    shutdown();
  }

  /// @brief Drains any queued tasks, then stops the worker thread.
  void shutdown() {
    if (!workerThread_.joinable()) {
      return;
    }
    sender_.send(T{});
    workerThread_.join();
  }

  /// @brief Get the const pointer to mutable sender of the SPSC channel
  auto *getSender() {
    return &sender_;
  }

 private:
  Receiver<T, Strategy, Wait> receiver_;
  Sender<T, Strategy, Wait> sender_;
  std::thread workerThread_;
};

} // namespace audioapi::task_offloader
