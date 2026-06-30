#pragma once

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace audioapi::slots {

/// Lock-free free-list of preallocated buffer slot indices (0..Capacity-1),
/// backed by a single atomic bitmask (bit i set == slot i is free).
///
/// Unlike an SPSC ring, acquire and release are safe from any thread: both the
/// audio thread (failure paths) and the worker thread release slots, so a
/// single-producer structure would be misused. Does not own audio data — only
/// tracks which pool index is available.
template <size_t Capacity>
class SlotFreeList {
 public:
  static_assert(Capacity > 0, "SlotFreeList requires Capacity > 0");
  static_assert(Capacity <= 64, "SlotFreeList stores slots in a 64-bit mask");

  /// Reserved slot value; TaskOffloader sends this on shutdown to unblock the worker.
  static constexpr size_t kSentinel = std::numeric_limits<size_t>::max();

  /// Mark all Capacity slots free. Call before handing the list to the audio thread.
  void seed() {
    freeMask_.store(kFullMask, std::memory_order_release);
  }

  /// Take a free slot index, or nullopt if the pool is exhausted. Multi-consumer safe.
  std::optional<size_t> tryAcquire() {
    uint64_t mask = freeMask_.load(std::memory_order_acquire);
    while (mask != 0) {
      const auto slot = static_cast<size_t>(std::countr_zero(mask));
      const uint64_t next = mask & (mask - 1); // clear lowest set bit
      if (freeMask_.compare_exchange_weak(
              mask, next, std::memory_order_acq_rel, std::memory_order_acquire)) {
        return slot;
      }
      // CAS failure refreshes `mask` with the current value; retry.
    }
    return std::nullopt;
  }

  /// Return a slot index. Multi-producer safe and never blocks, so it is safe on the
  /// real-time audio thread. A double-release just re-sets an already-free bit (no-op).
  void release(size_t slot) {
    if (slot >= Capacity) {
      return;
    }
    freeMask_.fetch_or(uint64_t{1} << slot, std::memory_order_acq_rel);
  }

 private:
  // Low `Capacity` bits set. Built top-down so Capacity == 64 needs no special case:
  // the static_asserts keep the shift amount in [0, 63], avoiding shift-by-64 UB.
  static constexpr uint64_t kFullMask = ~uint64_t{0} >> (64 - Capacity);

  std::atomic<uint64_t> freeMask_{0};
};

} // namespace audioapi::slots
