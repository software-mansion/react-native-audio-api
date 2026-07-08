#pragma once

#include <cstdint>
#include <cstring>
#include <iterator>
#include <type_traits>

namespace audioapi::utils::graph {

/// @brief Free-list-based pool for storing input edges as singly-linked lists.
///
/// Replaces `std::vector<std::uint32_t>` inside AudioGraph::Node with a
/// pool-allocated linked list, eliminating all heap allocations during
/// audio-thread processing (toposort + compaction + iteration).
///
/// Each slot is 8 bytes — a union of {val, next} (when in an input list)
/// or {next_free} (when on the free list). 32-bit indices are used
/// throughout instead of pointers.
///
/// ## Growth policy
/// Grow should not be called on audio thread, instead the main thread should allocate a buffer and
/// send it to adoptBuffer() through some asynchronous channel
///
/// @note Can address up to 2^32 − 2 slots (~4 billion).
class InputPool {
 public:
  /// Sentinel value meaning "no slot" / "end of list".
  static constexpr std::uint32_t kNull = UINT32_MAX;

  /// A single slot in the pool — either part of an input linked list
  /// or part of the free list.
  struct Slot {
    union {
      struct {
        std::uint32_t val;  // input node index (when in use)
        std::uint32_t next; // next slot in the input linked list
      };
      std::uint32_t next_free; // next slot on free list (overlaps val)
    };
  };

  static_assert(sizeof(Slot) == 8, "Slot must be 8 bytes");

  /// @brief Forward iterator over values in an input linked list.
  /// @tparam Const if true, dereferences to `std::uint32_t` (by value);
  ///               if false, to `std::uint32_t &` (mutable reference).
  template <bool Const>
  struct Iterator {
    using value_type = std::uint32_t;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using SlotPtr = std::conditional_t<Const, const Slot *, Slot *>;
    using reference = std::conditional_t<Const, std::uint32_t, std::uint32_t &>;

    SlotPtr slots;
    std::uint32_t current;

    reference operator*() const;
    Iterator &operator++();
    void operator++(int);
    bool operator==(const Iterator &other) const;
    bool operator!=(const Iterator &other) const;
  };

  /// @brief Range view over a linked list's values.
  /// @tparam Const if true, immutable view; if false, mutable view.
  template <bool Const>
  struct InputView {
    using SlotPtr = std::conditional_t<Const, const Slot *, Slot *>;

    SlotPtr slots;
    std::uint32_t head;

    [[nodiscard]] Iterator<Const> begin() const;
    [[nodiscard]] Iterator<Const> end() const;
  };

  // ── Lifecycle ───────────────────────────────────────────────────────────

  InputPool();
  ~InputPool();

  InputPool(const InputPool &) = delete;
  InputPool &operator=(const InputPool &) = delete;

  InputPool(InputPool &&other) noexcept;
  InputPool &operator=(InputPool &&other) noexcept;

  // ── Slot allocation ─────────────────────────────────────────────────────

  /// @brief Allocate a slot from the free list.
  /// If the free list is empty, grows the pool (allocation on current thread).
  std::uint32_t alloc();

  /// @brief Return a slot to the free list.
  void free(std::uint32_t idx);

  // ── Linked-list operations ──────────────────────────────────────────────
  // All take `head` by reference — the caller's stored head index.

  /// @brief Prepend a value to the front of the linked list.
  void push(std::uint32_t &head, std::uint32_t inputVal);

  /// @brief Remove the first occurrence of `inputVal` from the list.
  /// @return true if found and removed, false otherwise.
  bool remove(std::uint32_t &head, std::uint32_t inputVal);

  /// @brief Remove all elements where `pred(val)` returns true.
  template <typename Pred>
    requires(std::predicate<Pred, std::uint32_t>)
  void removeIf(std::uint32_t &head, Pred pred);

  /// @brief Free every slot in the list back to the free list.
  void freeAll(std::uint32_t &head);

  /// @brief Check if a linked list is empty.
  [[nodiscard]] static bool isEmpty(std::uint32_t head);

  // ── Iteration ───────────────────────────────────────────────────────────

  /// @brief Returns an immutable range over the values in the list starting at `head`.
  [[nodiscard]] InputView<true> view(std::uint32_t head) const;

  /// @brief Returns a mutable range over the values in the list starting at `head`.
  [[nodiscard]] InputView<false> mutableView(std::uint32_t head);

  // ── Pool management ─────────────────────────────────────────────────────

  /// @brief Current pool capacity (number of slots).
  [[nodiscard]] std::uint32_t capacity() const;

  /// @brief Adopt a pre-allocated buffer. Copies existing data, adds new
  /// slots to the free list, and returns the old buffer for disposal.
  /// @param newSlots newly allocated slot array (caller transfers ownership)
  /// @param newCapacity size of the new array (must be > current capacity)
  /// @return old slot array — caller must `delete[]` it (may be nullptr)
  Slot *adoptBuffer(Slot *newSlots, std::uint32_t newCapacity);

  /// @brief Grow the pool by allocating a new buffer on the current thread.
  /// Copies existing data and adds new slots to the free list.
  /// @param newCapacity desired capacity (must be > current capacity)
  void grow(std::uint32_t newCapacity);

 private:
  Slot *slots_ = nullptr;
  std::uint32_t capacity_ = 0;
  std::uint32_t free_head_ = kNull;
};

template <bool Const>
auto InputPool::Iterator<Const>::operator*() const -> reference {
  return slots[current].val;
}

template <bool Const>
auto InputPool::Iterator<Const>::operator++() -> Iterator & {
  current = slots[current].next;
  return *this;
}

template <bool Const>
void InputPool::Iterator<Const>::operator++(int) {
  ++*this;
}

template <bool Const>
bool InputPool::Iterator<Const>::operator==(const Iterator &other) const {
  return current == other.current;
}

template <bool Const>
bool InputPool::Iterator<Const>::operator!=(const Iterator &other) const {
  return current != other.current;
}

template <bool Const>
auto InputPool::InputView<Const>::begin() const -> Iterator<Const> {
  return {.slots = slots, .current = head};
}

template <bool Const>
auto InputPool::InputView<Const>::end() const -> Iterator<Const> {
  return {.slots = slots, .current = kNull};
}

template <typename Pred>
  requires(std::predicate<Pred, std::uint32_t>)
void InputPool::removeIf(std::uint32_t &head, Pred pred) {
  std::uint32_t *prev = &head;
  std::uint32_t curr = head;
  while (curr != kNull) {
    std::uint32_t nxt = slots_[curr].next;
    if (pred(slots_[curr].val)) {
      *prev = nxt;
      free(curr);
    } else {
      prev = &slots_[curr].next;
    }
    curr = nxt;
  }
}

} // namespace audioapi::utils::graph
