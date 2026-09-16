#pragma once

#include <audioapi/utils/Macros.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <new>
#include <set>
#include <utility>

namespace audioapi {

namespace detail {

/// @brief Fixed-capacity, in-object block pool with an intrusive free list.
/// Hands out fixed-size slots and recycles freed ones; never touches the heap.
/// @note Exhaustion throws std::bad_alloc, but neither throw is reachable
/// through BoundedPriorityQueue: the slot size is checked against the real node
/// type at compile time, and `push` refuses once `Capacity` slots are taken.
template <size_t Capacity, size_t BlockSize, size_t Align>
  requires(Capacity > 0) && (BlockSize >= sizeof(void *))
class FixedBlockPool {
 public:
  // Round the requested block size up to the pool alignment. A slot is always
  // >= sizeof(void*), so the free list is stored intrusively in freed slots.
  static constexpr size_t SLOT_SIZE = ((BlockSize + Align - 1) / Align) * Align;
  static constexpr size_t SLOT_ALIGN = Align;

  void *allocate(size_t bytes) {
    // The multiset allocates tree nodes one at a time, all the same size.
    if (bytes > SLOT_SIZE) [[unlikely]] {
      throw std::bad_alloc();
    }
    if (freeList_ != nullptr) {
      void *block = freeList_;
      freeList_ = nextOf(block);
      return block;
    }
    if (used_ >= Capacity) [[unlikely]] {
      throw std::bad_alloc();
    }
    return &storage_[used_++ * SLOT_SIZE];
  }

  void deallocate(void *block) noexcept {
    nextOf(block) = freeList_;
    freeList_ = block;
  }

 private:
  static void *&nextOf(void *block) noexcept {
    return *static_cast<void **>(block);
  }

  alignas(Align) std::array<std::byte, Capacity * SLOT_SIZE> storage_{};
  void *freeList_ = nullptr;
  size_t used_ = 0;
};

/// @brief Stateful allocator that draws node storage from a FixedBlockPool.
/// Satisfies the C++ Allocator requirements used by std::multiset.
/// @note std::multiset rebinds this to its own node type, so `allocate` is the
/// one place where the exact node layout of the standard library in use is
/// visible — which is where the pool's slot size is verified.
template <typename T, typename Pool>
class PoolAllocator {
 public:
  using value_type = T;

  template <typename U>
  struct rebind {
    using other = PoolAllocator<U, Pool>;
  };

  explicit PoolAllocator(Pool *pool) noexcept : pool_(pool) {}

  template <typename U>
  PoolAllocator(const PoolAllocator<U, Pool> &other) noexcept // NOLINT(runtime/explicit)
      : pool_(other.pool()) {}

  T *allocate(size_t n) {
    static_assert(
        sizeof(T) <= Pool::SLOT_SIZE,
        "BoundedPriorityQueue: this standard library's tree node is larger than the pool slot "
        "-- raise kNodeOverhead");
    static_assert(
        alignof(T) <= Pool::SLOT_ALIGN,
        "BoundedPriorityQueue: this standard library's tree node is over-aligned for the pool");
    return static_cast<T *>(pool_->allocate(n * sizeof(T)));
  }

  void deallocate(T *p, size_t /*n*/) noexcept {
    pool_->deallocate(p);
  }

  [[nodiscard]] Pool *pool() const noexcept {
    return pool_;
  }

  template <typename U>
  bool operator==(const PoolAllocator<U, Pool> &o) const noexcept {
    return pool_ == o.pool();
  }
  template <typename U>
  bool operator!=(const PoolAllocator<U, Pool> &o) const noexcept {
    return pool_ != o.pool();
  }

 private:
  Pool *pool_;
};

} // namespace detail

/// @brief A bounded priority queue with fixed capacity backed by an in-object block pool.
/// Elements are kept in ascending sorted order (smallest element at front).
/// All operations avoid heap allocation. Uses std::multiset under the hood
/// to maintain sorted order and provide efficient insertion and removal.
/// @tparam T The type of elements stored. Must be move-constructible.
/// @tparam Capacity The maximum number of elements. Must be positive.
/// @tparam Compare Comparator type, a strict weak ordering over T. Defaults to
/// std::less<T> (smallest element at front). Heterogeneous key lookups
/// (lowerBound / upperBound) additionally need a transparent comparator that
/// accepts the key type.
/// @note Stable: for equal keys, insertion order is preserved by std::multiset.
/// @note This implementation is NOT thread-safe.
template <typename T, size_t Capacity, typename Compare = std::less<T>>
  requires(Capacity > 0) && std::move_constructible<T>
class BoundedPriorityQueue {
 private:
  // A red-black tree node is the value plus three links and a colour, so four
  // pointers bound the container's own per-node fields. If some standard library
  // ever exceeds that, the static_assert in PoolAllocator::allocate fires -- the
  // rebound allocator sees the real node type -- so this is checked, not assumed.
  static constexpr size_t kNodeOverhead = 4 * sizeof(void *);
  static constexpr size_t kBlockSize = sizeof(T) + kNodeOverhead;
  // A node holds T and pointers and nothing else, so its alignment is the
  // stricter of the two; PoolAllocator::allocate asserts that against the
  // real node type.
  static constexpr size_t kBlockAlign = alignof(T) > alignof(void *) ? alignof(T) : alignof(void *);

  using PoolType = detail::FixedBlockPool<Capacity, kBlockSize, kBlockAlign>;
  using AllocType = detail::PoolAllocator<T, PoolType>;
  using SetType = std::multiset<T, Compare, AllocType>;

  // Members must be declared in this order: pool_ → set_.
  // set_ allocates its nodes from pool_, so pool_ must outlive it.
  PoolType pool_;
  SetType set_{Compare{}, AllocType{&pool_}};

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
    requires std::predicate<const Compare &, const T &, const Key &> &&
      std::predicate<const Compare &, const Key &, const T &>
  [[nodiscard]] SetType::iterator lowerBound(const Key &key) noexcept {
    return set_.lower_bound(key);
  }

  /// @brief Find the first element with key > the given key.
  /// @note For events with duplicate keys, upperBound returns the position after the last duplicate.
  /// @return An iterator to the first element with key > the given key, or end() if no such element exists.
  template <typename Key>
    requires std::predicate<const Compare &, const T &, const Key &> &&
      std::predicate<const Compare &, const Key &, const T &>
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
