#pragma once

#include <bit>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

namespace audioapi {

/// @brief A bounded priority queue (min-heap) with fixed capacity.
/// @tparam T The type of elements stored in the queue.
/// @tparam capacity_ The maximum number of elements. Must be a power of two greater than zero.
/// @tparam Compare Comparator type. Defaults to std::less<T> (min-heap: smallest element at top).
/// @note This implementation is NOT thread-safe.
/// @note Capacity must be a power of two and greater than zero.
template <typename T, size_t capacity_, typename Compare = std::less<T>>
class BoundedPriorityQueue {
 public:
  explicit BoundedPriorityQueue() : size_(0) {
    static_assert(isPowerOfTwo(capacity_), "BoundedPriorityQueue's capacity must be a power of 2");
    buffer_ = static_cast<T *>(
        ::operator new[](capacity_ * sizeof(T), static_cast<std::align_val_t>(alignof(T))));
  }

  ~BoundedPriorityQueue() {
    for (size_t i = 0; i < size_; ++i) {
      buffer_[i].~T();
    }
    ::operator delete[](buffer_, capacity_ * sizeof(T), static_cast<std::align_val_t>(alignof(T)));
  }

  BoundedPriorityQueue(const BoundedPriorityQueue &) = delete;
  BoundedPriorityQueue &operator=(const BoundedPriorityQueue &) = delete;

  /// @brief Push a value into the priority queue.
  /// @tparam U The type of the value to push.
  /// @param value The value to push.
  /// @return True if pushed successfully, false if the queue is full.
  template <typename U>
  bool push(U &&value) noexcept(std::is_nothrow_constructible_v<T, U &&>) {
    if (isFull()) [[unlikely]] {
      return false;
    }
    new (&buffer_[size_]) T(std::forward<U>(value));
    siftUp(size_);
    ++size_;
    return true;
  }

  /// @brief Pop the top (highest priority) element and retrieve it.
  /// @param out The popped element.
  /// @return True if popped successfully, false if the queue is empty.
  bool pop(T &out) noexcept(std::is_nothrow_move_constructible_v<T> &&
                            std::is_nothrow_destructible_v<T>) {
    if (isEmpty()) [[unlikely]] {
      return false;
    }
    out = std::move(buffer_[0]);
    buffer_[0].~T();
    --size_;
    if (size_ > 0) {
      new (&buffer_[0]) T(std::move(buffer_[size_]));
      buffer_[size_].~T();
      siftDown(0);
    }
    return true;
  }

  /// @brief Pop the top element without retrieving it.
  /// @return True if popped successfully, false if the queue is empty.
  bool pop() noexcept(std::is_nothrow_destructible_v<T>) {
    if (isEmpty()) [[unlikely]] {
      return false;
    }
    buffer_[0].~T();
    --size_;
    if (size_ > 0) {
      new (&buffer_[0]) T(std::move(buffer_[size_]));
      buffer_[size_].~T();
      siftDown(0);
    }
    return true;
  }

  /// @brief Peek at the top (highest priority) element without removing it.
  /// @return A const reference to the top element.
  [[nodiscard]] inline const T &peek() const noexcept {
    return buffer_[0];
  }

  /// @brief Peek at the top (highest priority) element without removing it.
  /// @return A mutable reference to the top element.
  [[nodiscard]] inline T &peekMut() noexcept {
    return buffer_[0];
  }

  /// @brief Check if the queue is empty.
  /// @return True if the queue is empty, false otherwise.
  [[nodiscard]] inline bool isEmpty() const noexcept {
    return size_ == 0;
  }

  /// @brief Check if the queue is full.
  /// @return True if the queue is full, false otherwise.
  [[nodiscard]] inline bool isFull() const noexcept {
    return size_ == capacity_;
  }

  /// @brief Get the number of elements in the queue.
  /// @return The current number of elements.
  [[nodiscard]] inline size_t size() const noexcept {
    return size_;
  }

  /// @brief Get the maximum capacity of the queue.
  /// @return The capacity.
  [[nodiscard]] inline size_t getCapacity() const noexcept {
    return capacity_;
  }

 private:
  T *buffer_;
  size_t size_;
  Compare compare_;

  static constexpr bool isPowerOfTwo(size_t n) {
    return std::has_single_bit(n);
  }

  void siftUp(size_t index) noexcept {
    while (index > 0) {
      size_t parent = (index - 1) / 2;
      if (compare_(buffer_[index], buffer_[parent])) {
        swapAt(index, parent);
        index = parent;
      } else {
        break;
      }
    }
  }

  void siftDown(size_t index) noexcept {
    while (true) {
      size_t left = 2 * index + 1;
      size_t right = 2 * index + 2;
      size_t top = index;

      if (left < size_ && compare_(buffer_[left], buffer_[top])) {
        top = left;
      }
      if (right < size_ && compare_(buffer_[right], buffer_[top])) {
        top = right;
      }
      if (top == index) {
        break;
      }
      swapAt(index, top);
      index = top;
    }
  }

  void swapAt(size_t a, size_t b) noexcept {
    T tmp(std::move(buffer_[a]));
    buffer_[a].~T();
    new (&buffer_[a]) T(std::move(buffer_[b]));
    buffer_[b].~T();
    new (&buffer_[b]) T(std::move(tmp));
  }
};

} // namespace audioapi
