#pragma once

#include <audioapi/utils/SpscChannel.hpp>
#include <atomic>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

namespace audioapi {

/// @brief A generic class to offload object destruction to a separate thread.
/// @tparam T The type of object to be destroyed.
template <typename T>
class AudioDestructor {
 public:
  AudioDestructor() : isExiting_(false) {
    auto [sender, receiver] = channels::spsc::channel<
        std::shared_ptr<T>,
        channels::spsc::OverflowStrategy::WAIT_ON_FULL,
        channels::spsc::WaitStrategy::ATOMIC_WAIT>(kChannelCapacity);
    sender_ = std::move(sender);
    workerHandle_ = std::thread(&AudioDestructor::process, this, std::move(receiver));
  }

  ~AudioDestructor() {
    isExiting_.store(true, std::memory_order_release);

    // We need to send a nullptr to unblock the receiver
    sender_.send(nullptr);
    if (workerHandle_.joinable()) {
      workerHandle_.join();
    }
  }

  /// @brief Adds an audio object to the deconstruction queue.
  /// @param object The audio object to be deconstructed.
  /// @return True if the node was successfully added, false otherwise.
  /// @note audio object does NOT get moved out if it is not successfully added.
  bool tryAddForDeconstruction(std::shared_ptr<T> &&object) {
    return sender_.try_send(std::move(object)) == channels::spsc::ResponseStatus::SUCCESS;
  }

 private:
  static constexpr size_t kChannelCapacity = 1024;

  std::thread workerHandle_;
  std::atomic<bool> isExiting_;

  using SenderType = channels::spsc::Sender<
      std::shared_ptr<T>,
      channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      channels::spsc::WaitStrategy::ATOMIC_WAIT>;

  using ReceiverType = channels::spsc::Receiver<
      std::shared_ptr<T>,
      channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      channels::spsc::WaitStrategy::ATOMIC_WAIT>;

  SenderType sender_;

  /// @brief Processes audio objects for deconstruction.
  /// @param receiver The receiver channel for incoming audio objects.
  void process(ReceiverType &&receiver) {
    auto rcv = std::move(receiver);
    while (!isExiting_.load(std::memory_order_acquire)) {
      rcv.receive();
    }
  }
};

#undef AUDIO_NODE_DESTRUCTOR_SPSC_OPTIONS

} // namespace audioapi
