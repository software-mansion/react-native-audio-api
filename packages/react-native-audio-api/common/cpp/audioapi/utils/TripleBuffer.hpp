#pragma once

#include <audioapi/core/utils/Constants.h>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <new>

namespace audioapi {

template <typename T, typename... Args>
concept ConstructibleFromCopyable = std::constructible_from<T, Args...> &&
    (std::copy_constructible<std::remove_cvref_t<Args>> && ...);

/// @brief A lock-free triple buffer for single producer and single consumer scenarios.
/// The producer can write to one buffer while the consumer reads from another buffer, and the third buffer is idle.
/// The producer can publish new data by swapping the back buffer with the idle buffer,
/// and the consumer can get the latest data by swapping the front buffer with the idle buffer if there is an update.
/// @tparam T The type of the buffer.
template <typename T>
class TripleBuffer {
 public:
  template <typename... Args>
    requires ConstructibleFromCopyable<T, Args...>
  explicit TripleBuffer(Args &&...args) {
    new (&buffers_[0]) T(args...);
    new (&buffers_[1]) T(args...);
    new (&buffers_[2]) T(args...);
  }

  ~TripleBuffer() {
    std::launder(reinterpret_cast<T *>(&buffers_[0]))->~T();
    std::launder(reinterpret_cast<T *>(&buffers_[1]))->~T();
    std::launder(reinterpret_cast<T *>(&buffers_[2]))->~T();
  }

  TripleBuffer(const TripleBuffer &) = delete;
  TripleBuffer &operator=(const TripleBuffer &) = delete;

  TripleBuffer(TripleBuffer &&) = delete;
  TripleBuffer &operator=(TripleBuffer &&) = delete;

  T *getForWriter() {
    return std::launder(reinterpret_cast<T *>(&buffers_[backIndex_]));
  }

  void publish() {
    State newState{backIndex_, true};
    auto prevState = state_.exchange(newState, std::memory_order_acq_rel);
    backIndex_ = prevState.index;
  }

  T *getForReader() {
    auto state = state_.load(std::memory_order_relaxed);
    if (state.hasUpdate) {
      State newState{frontIndex_, false};
      auto prevState = state_.exchange(newState, std::memory_order_acq_rel);
      frontIndex_ = prevState.index;
    }

    return std::launder(reinterpret_cast<T *>(&buffers_[frontIndex_]));
  }

 private:
  struct State {
    uint32_t index : 2;
    uint32_t hasUpdate : 1;
  };

  struct alignas(hardware_destructive_interference_size) AlignedBuffer {
    alignas(T) std::byte data[sizeof(T)];
  };

  AlignedBuffer buffers_[3];
  alignas(hardware_destructive_interference_size) uint32_t frontIndex_{0};
  alignas(hardware_destructive_interference_size) std::atomic<State> state_{{1, false}};
  alignas(hardware_destructive_interference_size) uint32_t backIndex_{2};
};

} // namespace audioapi
