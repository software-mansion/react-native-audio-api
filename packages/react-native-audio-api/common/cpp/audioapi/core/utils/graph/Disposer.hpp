#pragma once

#include <audioapi/utils/SpscChannel.hpp>

#include <thread>
#include <utility>

namespace audioapi::utils::graph {

class Disposer {
 public:
  virtual ~Disposer() = default;

  /// @brief Disposes the given pointer, correctly invoking the destructor for
  /// type T, regardless of the concrete type. The pointer is sent as void* to a
  /// worker thread where it is cast back to T* and deleted.
  /// @tparam T The original type of the object being disposed.
  /// @param ptr Pointer to the object to be disposed.
  template <typename T>
  void dispose(T *ptr) {
    if (ptr == nullptr) {
      return;
    }
    doDispose(static_cast<void *>(ptr), [](void *p) { delete static_cast<T *>(p); });
  }

 protected:
  /// @brief Type-erased disposal — subclasses implement this to transfer
  /// both the void* and the matching deleter to a worker thread.
  /// @param ptr The type-erased pointer.
  /// @param deleter A function that knows how to delete the original type.
  virtual void doDispose(void *ptr, void (*deleter)(void *)) = 0;
};

/// @brief Performs deallocation on a separate worker thread.
class DisposerImpl : public Disposer {
  struct Payload {
    void *ptr;
    void (*deleter)(void *);
  };

  using Receiver = audioapi::channels::spsc::Receiver<
      Payload,
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT>;
  using Sender = audioapi::channels::spsc::Sender<
      Payload,
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT>;

 public:
  DisposerImpl() {
    using namespace audioapi::channels::spsc;
    auto [sender, receiver] =
        channel<Payload, OverflowStrategy::WAIT_ON_FULL, WaitStrategy::ATOMIC_WAIT>(1024);
    sender_ = std::move(sender);
    workerHandle_ = std::thread([receiver = std::move(receiver)]() mutable {
      while (true) {
        auto payload = receiver.receive();
        if (payload.ptr == nullptr) {
          break; // Sentinel: shutdown signal
        }
        payload.deleter(payload.ptr);
      }
    });
  }

  ~DisposerImpl() override {
    // Send a sentinel payload to unblock and stop the worker thread
    sender_.send(Payload{nullptr, nullptr});
    if (workerHandle_.joinable()) {
      workerHandle_.join();
    }
  }

 protected:
  void doDispose(void *ptr, void (*deleter)(void *)) override {
    sender_.send(Payload{ptr, deleter});
  }

 private:
  Sender sender_;
  std::thread workerHandle_;
};

} // namespace audioapi::utils::graph
