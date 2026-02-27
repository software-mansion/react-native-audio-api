#pragma once

#include <atomic>
#include <utility>

namespace audioapi {

/// @brief A lock-free triple buffer for single producer and single consumer scenarios.
/// The producer can write to one buffer while the consumer reads from another buffer, and the third buffer is idle.
/// The producer can publish new data by swapping the back buffer with the idle buffer,
/// and the consumer can get the latest data by swapping the front buffer with the idle buffer if there is an update.
/// @tparam T The type of the buffer.
template <typename T>
class TripleBuffer {
 public:
  T *getForWriter() {
    return &buffers_[backIndex_];
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

    return &buffers_[frontIndex_];
  }

 private:
  struct State {
    int index;
    bool hasUpdate;
  };

  T buffers_[3];
  int frontIndex_ = 0;
  std::atomic<State> state_{{1, false}};
  int backIndex_ = 2;
};

} // namespace audioapi
