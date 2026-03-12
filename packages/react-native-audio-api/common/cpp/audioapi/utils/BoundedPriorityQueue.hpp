#pragma once

#include <bit>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

namespace audioapi {

/// @brief A bounded priority queue (min-heap) with fixed capacity. When full, new elements are rejected. When popping, the highest priority element is removed and returned.
/// @tparam T The type of elements stored in the queue.
/// @tparam capacity_ The maximum number of elements. Must be a power of two greater than zero.
/// @tparam Compare Comparator type. Defaults to std::less<T> (min-heap: smallest element at top). The queue implements a stable priority order meaning that if two elements compare equal, the one that was inserted earlier will be popped first.
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
    new (&buffer_[size_]) TimestampedElement(std::forward<U>(value), globalCounter_++);
    siftUp(size_);
    ++size_;
    return true;
  }

  /// @brief Pop the top (highest priority) element and retrieve it.
  /// @param out The popped element.
  /// @return True if popped successfully, false if the queue is empty.
  bool pop(T &out) noexcept(
      std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>) {
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
  [[nodiscard]] inline const T &peekFront() const noexcept {
    return buffer_[0];
  }

  /// @brief Peek at the last (lowest priority) element without removing it.
  /// @return A reference to the last element.
  [[nodiscard]] inline T &peekFrontMut() noexcept {
    return buffer_[size_ - 1];
  }

  /// @brief Peek at the last (lowest priority) element without removing it.
  /// @return A reference to the last element.
  [[nodiscard]] inline const T &peekBack() const noexcept {
    return buffer_[size_ - 1];
  }

  /// @brief Peek at the last (lowest priority) element without removing it.
  /// @return A reference to the last element.
  [[nodiscard]] inline T &peekBackMut() noexcept {
    return buffer_[size_ - 1];
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

  /// @brief Peek at the i-th element in the internal buffer (heap order, not sorted).
  /// @note Intended for iterating over all elements without removing them.
  [[nodiscard]] inline const T &peekAt(size_t i) const noexcept {
    return buffer_[i].data;
  }

 private:
  // Internal wrapper to track arrival order
  struct TimestampedElement {
    T data;
    uint64_t insertionOrder;

    // Use the provided Compare for T, but fall back to insertionOrder for ties
    struct InternalCompare {
      Compare userComp;
      bool operator()(const TimestampedElement &a, const TimestampedElement &b) const {
        if (userComp(a.data, b.data)) {
          return true;
        }
        if (userComp(b.data, a.data)) {
          return false;
        }
        return a.insertionOrder < b.insertionOrder;
      }
    };
  };

  TimestampedElement *buffer_;
  size_t size_;
  uint64_t globalCounter_ = 0;
  typename TimestampedElement::InternalCompare compare_;

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
