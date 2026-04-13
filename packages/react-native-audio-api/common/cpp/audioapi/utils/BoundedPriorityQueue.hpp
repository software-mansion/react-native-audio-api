#pragma once

#include <audioapi/utils/Macros.h>
#include <array>
#include <cstddef>
#include <functional>
#include <memory_resource>
#include <set>
#include <utility>

namespace audioapi {

/// @brief A bounded priority queue with fixed capacity backed by a static pool allocator.
/// Elements are kept in ascending sorted order (smallest element at front).
/// All operations avoid heap allocation. Uses std::multiset under the hood
// to maintain sorted order and provide efficient insertion and removal.
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
  DELETE_COPY_AND_MOVE(BoundedPriorityQueue);

  /// @brief Insert a value in sorted order. Amortized O(1) when inserting the largest element
  /// (common case: events scheduled in chronological order), O(log n) otherwise.
  /// @return True if inserted, false if full.
  template <typename U>
  bool push(U &&value) {
    if (isFull()) [[unlikely]] {
      return false;
    }
    // Hint with end(): amortized O(1) when the new event has the largest key (in-order scheduling).
    set_.insert(set_.end(), std::forward<U>(value));
    return true;
  }

  /// @brief Remove and return the smallest element (front).
  /// @return True if successful, false if empty.
  bool pop(T &out) {
    if (isEmpty()) [[unlikely]] {
      return false;
    }
    auto node = set_.extract(set_.begin());
    out = std::move(node.value());
    return true;
  }

  /// @brief Remove the smallest element (front) without retrieving it.
  /// @return True if successful, false if empty.
  bool pop() {
    if (isEmpty()) [[unlikely]] {
      return false;
    }
    set_.erase(set_.begin());
    return true;
  }

  /// @brief Peek at the smallest element (front).
  [[nodiscard]] const T &peekFront() const noexcept {
    return *set_.begin();
  }

  /// @brief Peek at the largest element (back).
  [[nodiscard]] const T &peekBack() const noexcept {
    return *std::prev(set_.end());
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

  [[nodiscard]] SetType::const_iterator begin() const noexcept {
    return set_.begin();
  }

  [[nodiscard]] SetType::const_iterator end() const noexcept {
    return set_.end();
  }

  /// @brief Find the first element with key >= the given key.
  /// @return An iterator to the first element with key >= the given key, or end() if no such element exists.
  template <typename Key>
  [[nodiscard]] SetType::iterator lowerBound(const Key &key) noexcept {
    return set_.lower_bound(key);
  }

  /// @brief Find the first element with key > the given key.
  /// @note For events with duplicate keys, upperBound returns the position after the last duplicate.
  /// @return An iterator to the first element with key > the given key, or end() if no such element exists.
  template <typename Key>
  [[nodiscard]] SetType::iterator upperBound(const Key &key) noexcept {
    return set_.upper_bound(key);
  }

  /// @brief Erase all elements in the range [first, last).
  void erase(SetType::iterator first, SetType::iterator last) noexcept {
    set_.erase(first, last);
  }

  /// @brief Extract a node at the given iterator position.
  /// @note The extracted node can be modified and reinserted without reallocation.
  /// @param it An iterator pointing to the element to extract. Must be a valid iterator in the set.
  /// @return A node handle owning the extracted element.
  [[nodiscard]] SetType::node_type extract(SetType::iterator it) noexcept {
    return set_.extract(it);
  }

  /// @brief Reinsert an extracted node with a hint.
  /// @note Use std::next(original_it) as hint when the sort key has not changed.
  /// @param hint An iterator pointing to the position before which the node should be reinserted. Must be a valid iterator in the set or end().
  /// @param node A node handle owning the element to reinsert. Must have been extracted from this set and not yet reinserted.
  void insert(SetType::iterator hint, SetType::node_type &&node) noexcept {
    set_.insert(hint, std::move(node));
  }
};

} // namespace audioapi
