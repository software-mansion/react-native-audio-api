#pragma once

#include <bit>
#include <memory>
#include <type_traits>

namespace audioapi {

/// @brief A ring buffer implementation (non thread safe).
/// @tparam T The type of elements stored in the buffer.
/// @tparam capacity_ The maximum number of elements that can be held in the buffer.
/// @note This implementation is NOT thread-safe.
/// @note Can be refered as bounded queue
/// @note Capacity must be a valid power of two and must be greater than zero.
template <typename T, size_t capacity_>
class RingBiDirectionalBuffer {
 public:
  /// @brief Constructor for RingBuffer.
  RingBiDirectionalBuffer()
    : headIndex_(0), tailIndex_(0) {
    static_assert(isPowerOfTwo(capacity_), "RingBiDirectionalBuffer's capacity must be power of 2");
    buffer_ = static_cast<T*>(
      ::operator new[](
        capacity_ * sizeof(T),
        static_cast<std::align_val_t>(alignof(T))
      )
    );
  }

  /// @brief Destructor for RingBuffer.
  ~RingBiDirectionalBuffer() {
    for (int i = headIndex_; i != tailIndex_; i = nextIndex(i)) {
      buffer_[i].~T();
    }
    ::operator delete[](
      buffer_,
      capacity_ * sizeof(T),
      static_cast<std::align_val_t>(alignof(T))
    );
  }

  /// @brief Push a value into the ring buffer.
  /// @tparam U The type of the value to push.
  /// @param value The value to push.
  /// @return True if the value was pushed successfully, false if the buffer is full.
  template <typename U>
  bool pushBack(U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
    if (isFull()) [[ unlikely ]] {
      return false;
    }
    new (&buffer_[tailIndex_]) T(std::forward<U>(value));
    tailIndex_ = nextIndex(tailIndex_);
    return true;
  }

  /// @brief Push a value to the front of the buffer.
  /// @tparam U The type of the value to push.
  /// @param value The value to push.
  /// @return True if the value was pushed successfully, false if the buffer is full.
  template <typename U>
  bool pushFront(U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
    if (isFull()) [[ unlikely ]] {
      return false;
    }
    headIndex_ = prevIndex(headIndex_);
    new (&buffer_[headIndex_]) T(std::forward<U>(value));
    return true;
  }

  /// @brief Pop a value from the front of the buffer.
  /// @param out The value popped from the buffer.
  /// @return True if the value was popped successfully, false if the buffer is empty.
  bool popFront(T& out) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>) {
    if (isEmpty()) [[ unlikely ]] {
      return false;
    }
    out = std::move(buffer_[headIndex_]);
    buffer_[headIndex_].~T();
    headIndex_ = nextIndex(headIndex_);
    return true;
  }

  /// @brief Pop a value from the front of the buffer.
  /// @return True if the value was popped successfully, false if the buffer is empty.
  bool popFront() noexcept(std::is_nothrow_destructible_v<T>) {
    if (isEmpty()) [[ unlikely ]] {
      return false;
    }
    buffer_[headIndex_].~T();
    headIndex_ = nextIndex(headIndex_);
    return true;
  }

  /// @brief Pop a value from the back of the buffer.
  /// @param out The value popped from the buffer.
  /// @return True if the value was popped successfully, false if the buffer is empty.
  bool popBack(T& out) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>) {
    if (isEmpty()) [[ unlikely ]] {
      return false;
    }
    tailIndex_ = prevIndex(tailIndex_);
    out = std::move(buffer_[tailIndex_]);
    buffer_[tailIndex_].~T();
    return true;
  }

  /// @brief Pop a value from the back of the buffer.
  /// @return True if the value was popped successfully, false if the buffer is empty.
  bool popBack() noexcept(std::is_nothrow_destructible_v<T>) {
    if (isEmpty()) [[ unlikely ]] {
      return false;
    }
    tailIndex_ = prevIndex(tailIndex_);
    buffer_[tailIndex_].~T();
    return true;
  }

  /// @brief Peek at the front of the buffer.
  /// @return A const reference to the front element of the buffer.
  const inline T& peekFront() const noexcept {
    return buffer_[headIndex_];
  }

  /// @brief Peek at the back of the buffer.
  /// @return A const reference to the back element of the buffer.
  const inline T& peekBack() const noexcept {
    return buffer_[prevIndex(tailIndex_)];
  }

  /// @brief Peek at the front of the buffer.
  /// @return A mutable reference to the front element of the buffer.
  inline T& peekFrontMut() noexcept {
    return buffer_[headIndex_];
  }

  /// @brief Peek at the back of the buffer.
  /// @return A mutable reference to the back element of the buffer.
  inline T& peekBackMut() noexcept {
    return buffer_[prevIndex(tailIndex_)];
  }

  /// @brief Check if the buffer is empty.
  /// @return True if the buffer is empty, false otherwise.
  inline bool isEmpty() const noexcept {
    return headIndex_ == tailIndex_;
  }

  /// @brief Check if the buffer is full.
  /// @return True if the buffer is full, false otherwise.
  inline bool isFull() const noexcept {
    return nextIndex(tailIndex_) == headIndex_;
  }

  /// @brief Get the capacity of the buffer.
  /// @return The capacity of the buffer.
  inline size_t getCapacity() const noexcept {
    return capacity_;
  }

  /// @brief Get the real capacity of the buffer (excluding one slot for the empty state).
  /// @return The real capacity of the buffer.
  inline size_t getRealCapacity() const noexcept {
    return capacity_ - 1;
  }

  /// @brief Get the number of elements in the buffer.
  /// @return The number of elements in the buffer.
  inline size_t size() const noexcept {
    return (capacity_ + tailIndex_ - headIndex_) & (capacity_ - 1);
  }

 private:
  T *buffer_;
  size_t headIndex_;
  size_t tailIndex_;

  /// @brief Get the next index in the buffer.
  /// @param n The current index.
  /// @return The next index in the buffer.
  inline size_t nextIndex(const size_t n) const noexcept {
    return (n + 1) & (capacity_ - 1);
  }

  /// @brief Get the previous index in the buffer.
  /// @param n The current index.
  /// @return The previous index in the buffer.
  inline size_t prevIndex(const size_t n) const noexcept {
    return (n - 1) & (capacity_ - 1);
  }

  /// @brief Check if a number is a power of two.
  /// @param n The number to check.
  /// @return True if n is a power of two, false otherwise.
  static constexpr bool isPowerOfTwo(size_t n) {
    return std::has_single_bit(n);
  }
};

};
