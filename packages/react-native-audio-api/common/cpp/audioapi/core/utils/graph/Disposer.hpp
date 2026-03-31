#pragma once

#include <audioapi/utils/SpscChannel.hpp>

#include <cstddef>
#include <cstring>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>

namespace audioapi::utils::graph {

/// @brief A disposal payload that can hold any trivially-relocatable or
/// move-constructible type up to N bytes. The value is moved into a raw byte
/// buffer, sent over a channel as plain data, and its destructor is called on
/// the worker thread via a type-erased function pointer.
/// @tparam N Maximum size in bytes of the object that can be stored.
template <size_t N>
struct DisposalPayload {
  alignas(std::max_align_t) std::byte data[N];
  void (*destructor)(void *); // type-erased destructor

  /// @brief Sentinel check — a null destructor means "shutdown".
  bool isSentinel() const;

  /// @brief Creates a sentinel payload used to signal worker thread shutdown.
  static DisposalPayload sentinel();

  /// @brief Creates a payload by move-constructing the value into the raw byte
  /// buffer.
  /// @tparam T The type of value to store. Must fit in N bytes.
  /// @param value The value to move into the payload.
  template <typename T>
  static DisposalPayload create(T &&value);

  /// @brief Invokes the stored destructor on the data buffer.
  void destroy();
};

/// @brief Abstract base for disposing objects on a separate thread.
/// @tparam N Maximum size in bytes of objects that can be disposed.
template <size_t N>
class Disposer {
 public:
  virtual ~Disposer() = default;

  /// @brief Disposes a value by moving it into a payload and sending it to the
  /// worker thread. The destructor will be called on the worker thread.
  /// @tparam T The type of value to dispose. Must be move-constructible and
  /// fit in N bytes.
  /// @param value The value to dispose (will be moved from).
  /// @return true if successfully enqueued, false if the channel was full.
  template <typename T>
  bool dispose(T &&value);

 protected:
  virtual bool doDispose(DisposalPayload<N> &&payload) = 0;
};

/// @brief Performs destruction on a separate worker thread.
/// @tparam N Maximum size in bytes of objects that can be disposed.
template <size_t N>
class DisposerImpl : public Disposer<N> {
  using Payload = DisposalPayload<N>;

  using Sender = audioapi::channels::spsc::Sender<
      Payload,
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT>;

 public:
  explicit DisposerImpl(size_t channelCapacity = 1024);
  ~DisposerImpl() override;

 protected:
  bool doDispose(DisposalPayload<N> &&payload) override;

 private:
  Sender sender_;
  std::thread workerHandle_;
};

// =========================================================================
// Implementation
// =========================================================================

// ── DisposalPayload ───────────────────────────────────────────────────────

template <size_t N>
bool DisposalPayload<N>::isSentinel() const {
  return destructor == nullptr;
}

template <size_t N>
DisposalPayload<N> DisposalPayload<N>::sentinel() {
  DisposalPayload p;
  p.destructor = nullptr;
  return p;
}

template <size_t N>
template <typename T>
DisposalPayload<N> DisposalPayload<N>::create(T &&value) {
  using DecayedT = std::decay_t<T>;
  static_assert(sizeof(DecayedT) <= N, "Type too large for DisposalPayload");
  static_assert(
      alignof(DecayedT) <= alignof(std::max_align_t), "Type alignment exceeds max_align_t");

  DisposalPayload p;
  // Move-construct the value into the raw buffer
  new (p.data) DecayedT(std::forward<T>(value));
  p.destructor = [](void *ptr) {
    static_cast<DecayedT *>(ptr)->~DecayedT();
  };
  return p;
}

template <size_t N>
void DisposalPayload<N>::destroy() {
  if (destructor != nullptr) {
    destructor(data);
  }
}

// ── Disposer ──────────────────────────────────────────────────────────────

template <size_t N>
template <typename T>
bool Disposer<N>::dispose(T &&value) {
  return doDispose(DisposalPayload<N>::template create<T>(std::forward<T>(value)));
}

// ── DisposerImpl ──────────────────────────────────────────────────────────

template <size_t N>
DisposerImpl<N>::DisposerImpl(size_t channelCapacity) {
  using namespace audioapi::channels::spsc;
  auto [sender, receiver] =
      channel<Payload, OverflowStrategy::WAIT_ON_FULL, WaitStrategy::ATOMIC_WAIT>(channelCapacity);
  sender_ = std::move(sender);
  workerHandle_ = std::thread([receiver = std::move(receiver)]() mutable {
    while (true) {
      auto payload = receiver.receive();
      if (payload.isSentinel()) {
        break;
      }
      payload.destroy();
    }
  });
}

template <size_t N>
DisposerImpl<N>::~DisposerImpl() {
  sender_.send(Payload::sentinel());
  if (workerHandle_.joinable()) {
    workerHandle_.join();
  }
}

template <size_t N>
bool DisposerImpl<N>::doDispose(DisposalPayload<N> &&payload) {
  return sender_.try_send(std::move(payload)) == audioapi::channels::spsc::ResponseStatus::SUCCESS;
}

} // namespace audioapi::utils::graph
