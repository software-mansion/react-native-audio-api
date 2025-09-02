#pragma once

#include <memory>
#include <type_traits>

namespace audioapi {

/// @brief A ring buffer implementation (non thread safe).
/// @tparam T The type of elements stored in the buffer.
/// @note This implementation is NOT thread-safe.
/// @note Can be refered as bounded queue
template <typename T>
class RingBuffer {
  static constexpr bool isPowerOfTwo(size_t n) {
    return (n & (n - 1)) == 0;
  }
 public:
  RingBuffer(size_t capacity)
    : headIndex_(0), tailIndex_(0) {
    static_assert(isPowerOfTwo(capacity), "RingBuffer's capacity must be power of 2");
    capacity_ = capacity;
    buffer_ = static_cast<T*>(
      ::operator new[](
        capacity_ * sizeof(T),
        static_cast<std::align_val_t>(alignof(T))
      )
    );
  }

  ~RingBuffer() {
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
  bool push(U&& value) noexcept(std::is_nothrow_constructible<T, U&&>::value) {
    if (isFull()) [[ unlikely ]] {
      return false;
    }
    new (&buffer_[tailIndex_]) T(std::forward<U>(value));
    tailIndex_ = nextIndex(tailIndex_);
    return true;
  }

  T pop() noexcept(std::is_nothrow_move_constructible<T>::value && std::is_nothrow_destructible<T>::value) {
    T value(std::move(buffer_[headIndex_]));
    buffer_[headIndex_].~T();
    headIndex_ = nextIndex(headIndex_);
  }

  const inline T& peekFront() const noexcept {
    return buffer_[headIndex_];
  }

  const inline T& peekBack() const noexcept {
    return buffer_[tailIndex_];
  }

  const inline bool isEmpty() const noexcept {
    return headIndex_ == tailIndex_;
  }

  const inline bool isFull() const noexcept {
    return nextIndex(tailIndex_) == headIndex_;
  }

  const inline size_t getCapacity() const noexcept {
    return capacity_;
  }

  const inline size_t getRealCapacity() const noexcept {
    return capacity_ - 1;
  }

  const inline size_t size() const noexcept {
    return (capacity_ + tailIndex_ - headIndex_) & (capacity_ - 1);
  }

 private:
  T *buffer_;
  size_t capacity_;
  size_t headIndex_;
  size_t tailIndex_;

  inline size_t nextIndex(const size_t n) const {
    return (n + 1) & (capacity_ - 1);
  }
};

};
