#pragma once

#include <audioapi/utils/SpscChannel.hpp>

#include <cstddef>
#include <limits>
#include <optional>
#include <utility>

namespace audioapi::slots {

/// Lock-free free-list of preallocated buffer slot indices (0..Capacity-1).
/// Audio thread acquires a slot, copies into pool[slot], worker releases it.
/// Does not own audio data — only tracks which pool index is available.
template <size_t Capacity>
class SlotFreeList {
 public:
  static_assert(Capacity > 0, "SlotFreeList requires Capacity > 0");

  /// Reserved slot value; TaskOffloader sends this on shutdown to unblock the worker.
  static constexpr size_t kSentinel = std::numeric_limits<size_t>::max();

  SlotFreeList() {
    // WAIT_ON_FULL SPSC rings hold at most (capacity - 1) elements. Request one
    // extra slot so `seed()` can enqueue all Capacity indices without blocking.
    auto [sender, receiver] = channels::spsc::channel<
        size_t,
        channels::spsc::OverflowStrategy::WAIT_ON_FULL,
        channels::spsc::WaitStrategy::ATOMIC_WAIT>(Capacity + 1);
    sender_ = std::move(sender);
    receiver_ = std::move(receiver);
  }

  /// JS thread: enqueue indices 0..Capacity-1 (all slots start free).
  void seed() {
    for (size_t i = 0; i < Capacity; ++i) {
      sender_.send(i);
    }
  }

  /// Audio thread: take a free slot index, or nullopt if the pool is exhausted.
  std::optional<size_t> tryAcquire() {
    size_t slot = 0;
    auto status = receiver_.try_receive(slot);
    if (status != channels::spsc::ResponseStatus::SUCCESS) {
      return std::nullopt;
    }

    return slot;
  }

  /// Worker thread: return a slot index after processing pool[slot].
  void release(size_t slot) {
    sender_.send(slot);
  }

 private:
  channels::spsc::Sender<
      size_t,
      channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      channels::spsc::WaitStrategy::ATOMIC_WAIT>
      sender_;
  channels::spsc::Receiver<
      size_t,
      channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      channels::spsc::WaitStrategy::ATOMIC_WAIT>
      receiver_;
};

} // namespace audioapi::slots
