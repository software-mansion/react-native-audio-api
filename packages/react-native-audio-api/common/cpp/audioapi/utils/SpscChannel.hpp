#pragma once

#include <algorithm>
#include <atomic>
#include <memory>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

namespace audioapi::channels::spsc {

/// @brief Overflow strategy for sender when the channel is full
enum class OverflowStrategy {
  /// @brief Block and wait for space (default behavior)
  WAIT_ON_FULL,

  /// @brief Overwrite the oldest unread element
  OVERWRITE_ON_FULL
};

/// @brief Wait strategy for receiver and sender when looping and trying
enum class WaitStrategy {
  /// @brief Busy loop waiting strategy
  /// @note should be used when low latency is required and channel is not expected to wait
  /// @note should be definitely used with OverflowStrategy::OVERWRITE_ON_FULL
  /// @note it uses `asm volatile ("" ::: "memory")` to prevent harmful compiler optimizations
  BUSY_LOOP,

  /// @brief Yielding waiting strategy
  /// @note should be used when low latency is not critical and channel is expected to wait
  /// @note it uses std::this_thread::yield under the hood
  YIELD,

  /// @brief Atomic waiting strategy
  /// @note should be used when low latency is required and channel is expected to wait for longer
  /// @note it uses std::atomic_wait under the hood
  ATOMIC_WAIT,
};

/// @brief Response status for channel operations
enum class ResponseStatus {
  SUCCESS,
  CHANNEL_FULL,
  CHANNEL_EMPTY,

  /// @brief Indicates that the last value is being overwritten or read so try fails
  /// @note This status is only returned if given channel supports OVERWRITE_ON_FULL strategy
  SKIP_DUE_TO_OVERWRITE
};

template <typename T, OverflowStrategy Strategy, WaitStrategy Wait>
class Sender;
template <typename T, OverflowStrategy Strategy, WaitStrategy Wait>
class Receiver;
template <typename T, OverflowStrategy Strategy, WaitStrategy Wait>
class InnerChannel;

/// @brief Create a bounded single-producer, single-consumer channel
/// @param capacity The minimum capacity of the channel, real capacity will be equal to the closest higher or equal power of two - 1. So for example, if capacity = 12 then channel will hold 15 elements.
/// @tparam T The type of values sent through the channel
/// @tparam Strategy The overflow strategy (default: WAIT_ON_FULL)
/// @tparam Wait The wait strategy used when looping and trying to send or receive (default: BUSY_LOOP)
/// @return A pair of sender and receiver for the channel
template <
    typename T,
    OverflowStrategy Strategy = OverflowStrategy::WAIT_ON_FULL,
    WaitStrategy Wait = WaitStrategy::BUSY_LOOP>
std::pair<Sender<T, Strategy, Wait>, Receiver<T, Strategy, Wait>> channel(size_t capacity) {
  auto channel = std::make_shared<InnerChannel<T, Strategy, Wait>>(capacity);
  return {Sender<T, Strategy, Wait>(channel), Receiver<T, Strategy, Wait>(channel)};
}

/// @brief Sender for a single-producer, single-consumer channel
/// @tparam T The type of values sent through the channel
/// @tparam Strategy The overflow strategy used by the channel
/// It allows to send values to the channel. It is designed to be used only from one thread at a time.
template <
    typename T,
    OverflowStrategy Strategy = OverflowStrategy::WAIT_ON_FULL,
    WaitStrategy Wait = WaitStrategy::BUSY_LOOP>
class Sender {
  /// Disallows sender creation outside of channel function
  explicit Sender(std::shared_ptr<InnerChannel<T, Strategy, Wait>> chan) : channel_(chan) {}

 public:
  /// @brief Default constructor
  /// @note required to have sender as class member
  Sender() = default;
  Sender(const Sender &) = delete;
  Sender &operator=(const Sender &) = delete;

  Sender &operator=(Sender &&other) noexcept {
    channel_ = std::move(other.channel_);
    return *this;
  }
  Sender(Sender &&other) noexcept : channel_(std::move(other.channel_)) {}

  /// @brief Try to send a value to the channel
  /// @param value The value to send
  /// @return ResponseStatus indicating the result of the operation
  /// @note this function is lock-free and wait-free
  template <typename U>
  ResponseStatus try_send(U &&value) noexcept(std::is_nothrow_constructible_v<T, U &&>) {
    return channel_->try_send(std::forward<U>(value));
  }

  /// @brief Send a value to the channel (copy version)
  /// @param value The value to send
  /// @note This function is lock-free but may block if the channel is full
  void send(const T &value) noexcept(std::is_nothrow_constructible_v<T, const T &>) {
    if (channel_->try_send(value) != ResponseStatus::SUCCESS) [[unlikely]] {
      do {
        if constexpr (Wait == WaitStrategy::YIELD) {
          std::this_thread::yield(); // Yield to allow other threads to run
        } else if constexpr (Wait == WaitStrategy::BUSY_LOOP) {
          asm volatile("" ::: "memory"); // Busy loop, just spin with compiler barrier
        } else if constexpr (Wait == WaitStrategy::ATOMIC_WAIT) {
          channel_->rcvCursor_.wait(channel_->rcvCursorCache_, std::memory_order_acquire);
        }
      } while (channel_->try_send(value) != ResponseStatus::SUCCESS);
    }
  }

  /// @brief Send a value to the channel (move version)
  /// @param value The value to send
  /// @note This function is lock-free but may block if the channel is full.
  void send(T &&value) noexcept(std::is_nothrow_move_constructible_v<T>) {
    if (channel_->try_send(std::move(value)) != ResponseStatus::SUCCESS) [[unlikely]] {
      do {
        if constexpr (Wait == WaitStrategy::YIELD) {
          std::this_thread::yield(); // Yield to allow other threads to run
        } else if constexpr (Wait == WaitStrategy::BUSY_LOOP) {
          asm volatile("" ::: "memory"); // Busy loop, just spin with compiler barrier
        } else if constexpr (Wait == WaitStrategy::ATOMIC_WAIT) {
          channel_->rcvCursor_.wait(channel_->rcvCursorCache_, std::memory_order_acquire);
        }
      } while (channel_->try_send(std::move(value)) != ResponseStatus::SUCCESS);
    }
  }

 private:
  std::shared_ptr<InnerChannel<T, Strategy, Wait>> channel_;

  friend std::pair<Sender<T, Strategy, Wait>, Receiver<T, Strategy, Wait>>
  channel<T, Strategy, Wait>(size_t capacity);
};

/// @brief Receiver for a single-producer, single-consumer channel
/// @tparam T The type of values sent through the channel
/// @tparam Strategy The overflow strategy used by the channel
/// It allows to receive values from the channel. It is designed to be used only from one thread at a time.
template <
    typename T,
    OverflowStrategy Strategy = OverflowStrategy::WAIT_ON_FULL,
    WaitStrategy Wait = WaitStrategy::BUSY_LOOP>
class Receiver {
  /// Disallows receiver creation outside of channel function
  explicit Receiver(std::shared_ptr<InnerChannel<T, Strategy, Wait>> chan) : channel_(chan) {}

 public:
  /// @brief Default constructor
  /// @note required to have receiver as class member
  Receiver() = default;
  Receiver(const Receiver &) = delete;
  Receiver &operator=(const Receiver &) = delete;

  Receiver &operator=(Receiver &&other) noexcept {
    channel_ = std::move(other.channel_);
    return *this;
  }
  Receiver(Receiver &&other) noexcept : channel_(std::move(other.channel_)) {}

  /// @brief Try to receive a value from the channel
  /// @param value The received value
  /// @return ResponseStatus indicating the result of the operation
  /// @note This function is lock-free and wait-free.
  ResponseStatus try_receive(T &value) noexcept(
      std::is_nothrow_move_assignable_v<T> && std::is_nothrow_destructible_v<T>) {
    return channel_->try_receive(value);
  }

  /// @brief Receive a value from the channel
  /// @return The received value
  /// @note This function is lock-free but may block if the channel is empty.
  T receive() noexcept(
      std::is_nothrow_default_constructible_v<T> && std::is_nothrow_move_assignable_v<T> &&
      std::is_nothrow_destructible_v<T>) {
    T value;
    if (channel_->try_receive(value) != ResponseStatus::SUCCESS) [[unlikely]] {
      do {
        if constexpr (Wait == WaitStrategy::YIELD) {
          std::this_thread::yield(); // Yield to allow other threads to run
        } else if constexpr (Wait == WaitStrategy::BUSY_LOOP) {
          asm volatile("" ::: "memory"); // Busy loop, just spin with compiler barrier
        } else if constexpr (Wait == WaitStrategy::ATOMIC_WAIT) {
          channel_->sendCursor_.wait(channel_->sendCursorCache_, std::memory_order_acquire);
        }
      } while (channel_->try_receive(value) != ResponseStatus::SUCCESS);
    }
    return value;
  }

 private:
  std::shared_ptr<InnerChannel<T, Strategy, Wait>> channel_;

  friend std::pair<Sender<T, Strategy, Wait>, Receiver<T, Strategy, Wait>>
  channel<T, Strategy, Wait>(size_t capacity);
};

/// @brief Inner channel implementation for the SPSC queue
/// @tparam T The type of values sent through the channel
/// @tparam Strategy The overflow strategy to use when the channel is full
/// @tparam Wait The wait strategy used for internal operations
/// This class is not intended to be used directly by users.
/// @note this class is not thread safe and should be wrapped in std::shared_ptr
template <
    typename T,
    OverflowStrategy Strategy = OverflowStrategy::WAIT_ON_FULL,
    WaitStrategy Wait = WaitStrategy::BUSY_LOOP>
class InnerChannel {
 public:
  /// @brief Construct a channel with a given capacity
  /// @param capacity The minimum capacity of the channel, for performance it will be allocated with next power of 2
  /// Uses raw memory allocation so the T type is not required to provide default constructors
  /// alignment is the key for performance it makes sure that objects are properly aligned in memory for faster access
  explicit InnerChannel(size_t capacity)
      : capacity_(next_power_of_2(capacity)),
        capacity_mask_(capacity_ - 1),
        buffer_(
            static_cast<T *>(operator new[](capacity_ * sizeof(T), std::align_val_t{alignof(T)}))) {

    // Initialize cache values for better performance
    rcvCursorCache_ = 0;
    sendCursorCache_ = 0;

    // Initialize reader state for overwrite strategy
    if constexpr (Strategy == OverflowStrategy::OVERWRITE_ON_FULL) {
      oldestOccupied_.store(false, std::memory_order_relaxed);
    }
  }

  /// This should not be called if there is existing handle to reader or writer
  ~InnerChannel() {
    size_t sendCursor = sendCursor_.load(std::memory_order_seq_cst);
    size_t rcvCursor = rcvCursor_.load(std::memory_order_seq_cst);

    // Call destructors for all elements in the buffer
    size_t i = rcvCursor;
    while (i != sendCursor) {
      buffer_[i].~T();
      i = next_index(i);
    }

    // Deallocate the buffer
    ::operator delete[](buffer_, capacity_ * sizeof(T), std::align_val_t{alignof(T)});
  }

  /// @brief Try to send a value to the channel
  /// @param value The value to send
  /// @return ResponseStatus indicating the result of the operation
  /// @note This function is lock-free and wait-free
  template <typename U>
  ResponseStatus try_send(U &&value) noexcept(std::is_nothrow_constructible_v<T, U &&>) {
    if constexpr (Strategy == OverflowStrategy::WAIT_ON_FULL) {
      return try_send_wait_on_full(std::forward<U>(value));
    } else {
      return try_send_overwrite_on_full(std::forward<U>(value));
    }
  }

  /// @brief Try to receive a value from the channel
  /// @param value The variable to store the received value
  /// @return ResponseStatus indicating the result of the operation
  /// @note This function is lock-free and wait-free
  ResponseStatus try_receive(T &value) noexcept(
      std::is_nothrow_move_assignable_v<T> && std::is_nothrow_destructible_v<T>) {
    if constexpr (Strategy == OverflowStrategy::OVERWRITE_ON_FULL) {
      // Set reader active flag to prevent overwrites during read
      bool isOccupied = oldestOccupied_.exchange(true, std::memory_order_acq_rel);
      if (isOccupied) {
        // It means that the oldest element is being overwritten so we cannot read
        return ResponseStatus::SKIP_DUE_TO_OVERWRITE;
      }
    }

    size_t rcvCursor =
        rcvCursor_.load(std::memory_order_relaxed); // only receiver thread reads this

    if (rcvCursor == sendCursorCache_) {
      // Refresh cache
      sendCursorCache_ = sendCursor_.load(std::memory_order_acquire);
      if (rcvCursor == sendCursorCache_) {
        if constexpr (Strategy == OverflowStrategy::OVERWRITE_ON_FULL) {
          oldestOccupied_.store(false, std::memory_order_release);
        }

        return ResponseStatus::CHANNEL_EMPTY;
      }
    }

    value = std::move(buffer_[rcvCursor]);
    buffer_[rcvCursor].~T(); // Call destructor

    rcvCursor_.store(next_index(rcvCursor), std::memory_order_release);

    if constexpr (Wait == WaitStrategy::ATOMIC_WAIT) {
      rcvCursor_.notify_one(); // Notify sender that a value has been received
    }

    if constexpr (Strategy == OverflowStrategy::OVERWRITE_ON_FULL) {
      oldestOccupied_.store(false, std::memory_order_release);
    }

    return ResponseStatus::SUCCESS;
  }

 private:
  /// @brief Try to send with WAIT_ON_FULL strategy (original behavior)
  template <typename U>
  inline ResponseStatus try_send_wait_on_full(U &&value) noexcept(
      std::is_nothrow_constructible_v<T, U &&>) {
    size_t sendCursor =
        sendCursor_.load(std::memory_order_relaxed); // only sender thread writes this
    size_t next_sendCursor = next_index(sendCursor);

    if (next_sendCursor == rcvCursorCache_) {
      // Refresh the cache
      rcvCursorCache_ = rcvCursor_.load(std::memory_order_acquire);
      if (next_sendCursor == rcvCursorCache_)
        return ResponseStatus::CHANNEL_FULL;
    }

    // Construct the new element in place
    new (&buffer_[sendCursor]) T(std::forward<U>(value));

    sendCursor_.store(next_sendCursor, std::memory_order_release);

    if constexpr (Wait == WaitStrategy::ATOMIC_WAIT) {
      sendCursor_.notify_one(); // Notify receiver that a value has been sent
    }

    return ResponseStatus::SUCCESS;
  }

  /// @brief Try to send with OVERWRITE_ON_FULL strategy
  template <typename U>
  inline ResponseStatus try_send_overwrite_on_full(U &&value) noexcept(
      std::is_nothrow_constructible_v<T, U &&>) {
    size_t sendCursor =
        sendCursor_.load(std::memory_order_relaxed); // only sender thread writes this
    size_t next_sendCursor = next_index(sendCursor);

    if (next_sendCursor == rcvCursorCache_) {
      // Refresh the cache
      rcvCursorCache_ = rcvCursor_.load(std::memory_order_acquire);
      if (next_sendCursor == rcvCursorCache_) {
        bool isOldestOccupied = oldestOccupied_.exchange(true, std::memory_order_acq_rel);
        if (isOldestOccupied) {
          // If the oldest element is occupied, we cannot overwrite
          return ResponseStatus::SKIP_DUE_TO_OVERWRITE;
        }

        size_t newestRcvCursor = rcvCursor_.load(std::memory_order_acquire);

        /// If the receiver did not advance, we can safely advance the cursor
        if (rcvCursorCache_ == newestRcvCursor) {
          rcvCursorCache_ = next_index(newestRcvCursor);
          rcvCursor_.store(rcvCursorCache_, std::memory_order_release);
        } else {
          rcvCursorCache_ = newestRcvCursor;
        }

        oldestOccupied_.store(false, std::memory_order_release);
      }
    }

    // Normal case: buffer not full
    new (&buffer_[sendCursor]) T(std::forward<U>(value));
    sendCursor_.store(next_sendCursor, std::memory_order_release);

    if constexpr (Wait == WaitStrategy::ATOMIC_WAIT) {
      sendCursor_.notify_one(); // Notify receiver that a value has been sent
    }

    return ResponseStatus::SUCCESS;
  }

  /// @brief Calculate the next power of 2 greater than or equal to n
  /// @param n The input value
  /// @return The next power of 2
  static constexpr size_t next_power_of_2(const size_t n) noexcept {
    if (n <= 1)
      return 1;

    // Use bit manipulation for efficiency
    size_t power = 1;
    while (power < n) {
      power <<= 1;
    }
    return power;
  }

  /// @brief Get the next index in a circular buffer
  /// @param val The current index
  /// @return The next index
  /// @note it might not be used for performance but it is a good reference
  inline size_t next_index(const size_t val) const noexcept {
    return (val + 1) & capacity_mask_;
  }

  const size_t capacity_;
  const size_t capacity_mask_; // mask for bitwise next_index
  T *buffer_;

  /// Producer-side data (accessed by sender thread)
  alignas(hardware_constructive_interference_size) std::atomic<size_t> sendCursor_{0};
  alignas(hardware_constructive_interference_size) size_t rcvCursorCache_{
      0}; // reduces cache coherency

  /// Consumer-side data (accessed by receiver thread)
  alignas(hardware_constructive_interference_size) std::atomic<size_t> rcvCursor_{0};
  alignas(hardware_constructive_interference_size) size_t sendCursorCache_{
      0}; // reduces cache coherency

  /// Flag indicating if the oldest element is occupied
  alignas(hardware_constructive_interference_size) std::atomic<bool> oldestOccupied_{false};

  friend class Sender<T, Strategy, Wait>;
  friend class Receiver<T, Strategy, Wait>;
};

} // namespace audioapi::channels::spsc
