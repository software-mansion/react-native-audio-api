#pragma once

#include <audioapi/utils/Macros.h>
#include <array>
#include <cstddef>
#include <functional>
#include <set>
#include <utility>

// --- std::pmr availability gate ---------------------------------------------
// std::pmr's runtime symbols ship in Apple's system libc++ only from
// iOS 17.0 / macOS 14.0 / tvOS 17.0 / watchOS 10.0 onward
// (_LIBCPP_AVAILABILITY_HAS_PMR maps to _LIBCPP_INTRODUCED_IN_LLVM_16).
// libc++ currently leaves the pmr availability *markup* empty
// (see llvm/llvm-project#40340), so using std::pmr below those versions
// compiles without any diagnostic but fails at dyld load time:
//   Symbol not found: std::__1::pmr::memory_resource::~memory_resource()
// On those targets we fall back to a std::multiset backed by an in-object,
// non-pmr pool allocator that preserves the original design intent: a fixed
// buffer, node recycling, and no heap allocation (audio-thread / real-time
// safe). Define AUDIOAPI_HAS_PMR before including this header to override.
#if !defined(AUDIOAPI_HAS_PMR)
#if defined(_LIBCPP_VERSION) && defined(_LIBCPP_AVAILABILITY_HAS_PMR) && \
    !_LIBCPP_AVAILABILITY_HAS_PMR
#define AUDIOAPI_HAS_PMR 0
#else
#define AUDIOAPI_HAS_PMR 1
#endif
#endif

#if AUDIOAPI_HAS_PMR
#include <memory_resource>
#else
#include <new>
#endif

namespace audioapi {

#if !AUDIOAPI_HAS_PMR
namespace no_pmr {

/// @brief Fixed-capacity, in-object block pool with an intrusive free list.
/// Hands out fixed-size slots and recycles freed ones; never touches the heap.
/// Throws std::bad_alloc on exhaustion, matching the std::pmr path's
/// null_memory_resource upstream behaviour.
template <std::size_t Capacity, std::size_t BlockSize, std::size_t Align>
class FixedBlockPool {
 public:
  void *allocate(std::size_t bytes) {
    // The multiset allocates tree nodes one at a time, all the same size.
    if (bytes > kSlotSize) [[unlikely]] {
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
    return &storage_[used_++ * kSlotSize];
  }

  void deallocate(void *block) noexcept {
    nextOf(block) = freeList_;
    freeList_ = block;
  }

 private:
  // Round the requested block size up to the pool alignment. A slot is always
  // >= sizeof(void*), so the free list is stored intrusively in freed slots.
  static constexpr std::size_t kSlotSize = ((BlockSize + Align - 1) / Align) * Align;

  static void *&nextOf(void *block) noexcept {
    return *static_cast<void **>(block);
  }

  alignas(Align) std::array<std::byte, Capacity * kSlotSize> storage_{};
  void *freeList_ = nullptr;
  std::size_t used_ = 0;
};

/// @brief Stateful allocator that draws node storage from a FixedBlockPool.
/// Satisfies the C++ Allocator requirements used by std::multiset.
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

  T *allocate(std::size_t n) {
    return static_cast<T *>(pool_->allocate(n * sizeof(T)));
  }

  void deallocate(T *p, std::size_t /*n*/) noexcept {
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

} // namespace no_pmr
#endif // !AUDIOAPI_HAS_PMR

/// @brief A bounded priority queue with fixed capacity backed by a static pool allocator.
/// Elements are kept in ascending sorted order (smallest element at front).
/// All operations avoid heap allocation. Uses std::multiset under the hood
/// to maintain sorted order and provide efficient insertion and removal.
/// @tparam T The type of elements stored. Must be move-constructible.
/// @tparam Capacity The maximum number of elements.
/// @tparam Compare Comparator type. Defaults to std::less<T> (smallest element at front).
/// @note Stable: for equal keys, insertion order is preserved by std::multiset.
/// @note This implementation is NOT thread-safe.
template <typename T, size_t Capacity, typename Compare = std::less<T>>
class BoundedPriorityQueue {
 private:
#if AUDIOAPI_HAS_PMR
  using SetType = std::pmr::multiset<T, Compare>;

  static constexpr size_t kNodeOverhead = 96;
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
      std::pmr::pool_options{.max_blocks_per_chunk = 1, .largest_required_pool_block = 0},
      &mono_};
  SetType set_{Compare{}, &pool_};
#else
  static constexpr size_t kNodeOverhead = 96;
  // Upper bound on a std::multiset red-black-tree node holding a T (value plus
  // tree links/color). Generous headroom, matching the std::pmr path above.
  static constexpr size_t kBlockSize = sizeof(T) + kNodeOverhead;

  using PoolType = no_pmr::FixedBlockPool<Capacity, kBlockSize, alignof(std::max_align_t)>;
  using AllocType = no_pmr::PoolAllocator<T, PoolType>;
  using SetType = std::multiset<T, Compare, AllocType>;

  // Members must be declared in this order: pool_ → set_.
  // set_ allocates its nodes from pool_, so pool_ must outlive it.
  PoolType pool_;
  SetType set_{Compare{}, AllocType{&pool_}};
#endif

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
