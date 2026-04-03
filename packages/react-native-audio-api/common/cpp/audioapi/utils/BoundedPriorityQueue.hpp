#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <memory_resource>
#include <set>
#include <utility>

namespace audioapi {

/// @brief A bounded priority queue with fixed capacity backed by a static pool allocator.
/// Elements are kept in ascending sorted order (smallest element at front).
/// All operations avoid heap allocation.
/// @tparam T The type of elements stored. Must be move-constructible.
/// @tparam Capacity The maximum number of elements.
/// @tparam Compare Comparator type. Defaults to std::less<T> (smallest element at front).
/// @note Stable: for equal keys, insertion order is preserved by std::multiset.
/// @note This implementation is NOT thread-safe.
template <typename T, size_t Capacity, typename Compare = std::less<T>>
class BoundedPriorityQueue {
 private:
  using SetType = std::pmr::multiset<T, Compare>;

  // Conservative RB-tree node size: value + 3 pointers + color, aligned to pointer size.
  static constexpr size_t kNodeOverhead = 4 * sizeof(void *);
  static constexpr size_t kNodeSize = sizeof(T) + kNodeOverhead;
  // Extra headroom for pool resource bookkeeping structures.
  static constexpr size_t kBufferSize = Capacity * kNodeSize + 256;

  // Members must be declared in this order: buffer_ → mono_ → pool_ → set_.
  alignas(std::max_align_t) std::array<std::byte, kBufferSize> buffer_;
  std::pmr::monotonic_buffer_resource mono_{
      buffer_.data(),
      sizeof(buffer_),
      std::pmr::null_memory_resource()};
  std::pmr::unsynchronized_pool_resource pool_{
      std::pmr::pool_options{
          .max_blocks_per_chunk = Capacity,
          .largest_required_pool_block = kNodeSize},
      &mono_};
  SetType set_{Compare{}, &pool_};

 public:
  explicit BoundedPriorityQueue() = default;
  ~BoundedPriorityQueue() = default;

  BoundedPriorityQueue(const BoundedPriorityQueue &) = delete;
  BoundedPriorityQueue &operator=(const BoundedPriorityQueue &) = delete;
  BoundedPriorityQueue(BoundedPriorityQueue &&) noexcept = delete;
  BoundedPriorityQueue &operator=(BoundedPriorityQueue &&) noexcept = delete;

  /// @brief Insert a value in sorted order. Amortized O(1) when inserting the largest element
  /// (common case: events scheduled in chronological order), O(log n) otherwise.
  /// @return True if inserted, false if full.
  template <typename U>
  bool push(U &&value) {
    if (isFull()) {
      [[unlikely]] return false;
    }
    // Hint with end(): amortized O(1) when the new event has the largest key (in-order scheduling).
    set_.insert(set_.end(), std::forward<U>(value));
    return true;
  }

  /// @brief Remove and return the smallest element (front). Amortized O(1).
  /// @return True if successful, false if empty.
  bool pop(T &out) {
    if (isEmpty()) {
      [[unlikely]] return false;
    }
    auto node = set_.extract(set_.begin());
    out = std::move(node.value());
    return true;
  }

  /// @brief Remove the smallest element (front) without retrieving it. Amortized O(1).
  /// @return True if successful, false if empty.
  bool pop() {
    if (isEmpty()) {
      [[unlikely]] return false;
    }
    set_.erase(set_.begin());
    return true;
  }

  /// @brief Remove the largest element (back). Amortized O(1).
  /// @return True if successful, false if empty.
  bool popBack() {
    if (isEmpty()) {
      [[unlikely]] return false;
    }
    set_.erase(std::prev(set_.end()));
    return true;
  }

  /// @brief Peek at the smallest element (front).
  [[nodiscard]] const T &peekFront() const noexcept {
    return *set_.begin();
  }

  /// @brief Peek at the smallest element (front), mutable.
  [[nodiscard]] T &peekFrontMut() noexcept {
    return const_cast<T &>(*set_.begin());
  }

  /// @brief Peek at the largest element (back).
  [[nodiscard]] const T &peekBack() const noexcept {
    return *std::prev(set_.end());
  }

  /// @brief Peek at the largest element (back), mutable.
  [[nodiscard]] T &peekBackMut() noexcept {
    return const_cast<T &>(*std::prev(set_.end()));
  }

  [[nodiscard]] bool isEmpty() const noexcept {
    return set_.empty();
  }

  [[nodiscard]] bool isFull() const noexcept {
    return set_.size() >= Capacity;
  }

  [[nodiscard]] size_t size() const noexcept {
    return set_.size();
  }

  [[nodiscard]] size_t getCapacity() const noexcept {
    return Capacity;
  }

  [[nodiscard]] SetType::const_iterator begin() const noexcept {
    return set_.begin();
  }

  [[nodiscard]] SetType::const_iterator end() const noexcept {
    return set_.end();
  }

  template <typename Key>
  [[nodiscard]] SetType::iterator lower_bound(const Key &key) noexcept {
    return set_.lower_bound(key);
  }

  template <typename Key>
  [[nodiscard]] SetType::iterator upper_bound(const Key &key) noexcept {
    return set_.upper_bound(key);
  }

  [[nodiscard]] T &deref_mut(SetType::const_iterator it) noexcept {
    return const_cast<T &>(*it);
  }
};

} // namespace audioapi
