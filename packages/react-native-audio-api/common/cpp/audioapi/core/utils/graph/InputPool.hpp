#pragma once

#include <cstdint>
#include <cstring>
#include <iterator>
#include <new>

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
/// The pool auto-grows when the free list is exhausted. Growth allocates
/// on the current thread, so it should ideally only happen during
/// `processEvents()` (unguarded). HostGraph can pre-grow the pool via
/// `adoptBuffer()` to avoid even that.
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

  // ── Lifecycle ───────────────────────────────────────────────────────────

  InputPool() = default;

  ~InputPool() {
    delete[] slots_;
  }

  InputPool(const InputPool &) = delete;
  InputPool &operator=(const InputPool &) = delete;

  InputPool(InputPool &&other) noexcept
      : slots_(other.slots_), capacity_(other.capacity_), free_head_(other.free_head_) {
    other.slots_ = nullptr;
    other.capacity_ = 0;
    other.free_head_ = kNull;
  }

  InputPool &operator=(InputPool &&other) noexcept {
    if (this != &other) {
      delete[] slots_;
      slots_ = other.slots_;
      capacity_ = other.capacity_;
      free_head_ = other.free_head_;
      other.slots_ = nullptr;
      other.capacity_ = 0;
      other.free_head_ = kNull;
    }
    return *this;
  }

  // ── Slot allocation ─────────────────────────────────────────────────────

  /// @brief Allocate a slot from the free list.
  /// If the free list is empty, grows the pool (allocation on current thread).
  std::uint32_t alloc() {
    if (free_head_ == kNull) {
      grow(capacity_ == 0 ? 64 : capacity_ * 2);
    }
    std::uint32_t idx = free_head_;
    free_head_ = slots_[idx].next_free;
    return idx;
  }

  /// @brief Return a slot to the free list.
  void free(std::uint32_t idx) {
    slots_[idx].next_free = free_head_;
    free_head_ = idx;
  }

  // ── Linked-list operations ──────────────────────────────────────────────
  // All take `head` by reference — the caller's stored head index.

  /// @brief Prepend a value to the front of the linked list.
  void push(std::uint32_t &head, std::uint32_t inputVal) {
    std::uint32_t idx = alloc();
    slots_[idx].val = inputVal;
    slots_[idx].next = head;
    head = idx;
  }

  /// @brief Remove the first occurrence of `inputVal` from the list.
  /// @return true if found and removed, false otherwise.
  bool remove(std::uint32_t &head, std::uint32_t inputVal) {
    std::uint32_t *prev = &head;
    std::uint32_t curr = head;
    while (curr != kNull) {
      if (slots_[curr].val == inputVal) {
        *prev = slots_[curr].next;
        free(curr);
        return true;
      }
      prev = &slots_[curr].next;
      curr = slots_[curr].next;
    }
    return false;
  }

  /// @brief Remove all elements where `pred(val)` returns true.
  template <typename Pred>
  void removeIf(std::uint32_t &head, Pred pred) {
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

  /// @brief Free every slot in the list back to the free list.
  void freeAll(std::uint32_t &head) {
    while (head != kNull) {
      std::uint32_t nxt = slots_[head].next;
      free(head);
      head = nxt;
    }
  }

  /// @brief Check if a linked list is empty.
  [[nodiscard]] static bool isEmpty(std::uint32_t head) {
    return head == kNull;
  }

  // ── Iteration ───────────────────────────────────────────────────────────

  /// @brief Forward iterator over the values in an input linked list.
  struct Iterator {
    using value_type = std::uint32_t;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using pointer = const std::uint32_t *;
    using reference = std::uint32_t;

    const Slot *slots;
    std::uint32_t current;

    std::uint32_t operator*() const {
      return slots[current].val;
    }

    Iterator &operator++() {
      current = slots[current].next;
      return *this;
    }

    void operator++(int) {
      ++*this;
    }

    bool operator==(const Iterator &other) const {
      return current == other.current;
    }

    bool operator!=(const Iterator &other) const {
      return current != other.current;
    }
  };

  /// @brief Mutable forward iterator — dereferences to a mutable reference to val.
  struct MutableIterator {
    using value_type = std::uint32_t;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using pointer = std::uint32_t *;
    using reference = std::uint32_t &;

    Slot *slots;
    std::uint32_t current;

    std::uint32_t &operator*() const {
      return slots[current].val;
    }

    MutableIterator &operator++() {
      current = slots[current].next;
      return *this;
    }

    void operator++(int) {
      ++*this;
    }

    bool operator==(const MutableIterator &other) const {
      return current == other.current;
    }

    bool operator!=(const MutableIterator &other) const {
      return current != other.current;
    }
  };

  /// @brief Immutable range view over a linked list's values.
  struct InputView {
    const Slot *slots;
    std::uint32_t head;

    [[nodiscard]] Iterator begin() const {
      return {slots, head};
    }
    [[nodiscard]] Iterator end() const {
      return {slots, kNull};
    }
  };

  /// @brief Mutable range view over a linked list's values.
  struct MutableInputView {
    Slot *slots;
    std::uint32_t head;

    [[nodiscard]] MutableIterator begin() const {
      return {slots, head};
    }
    [[nodiscard]] MutableIterator end() const {
      return {slots, kNull};
    }
  };

  /// @brief Returns an immutable range over the values in the list starting at `head`.
  [[nodiscard]] InputView view(std::uint32_t head) const {
    return {slots_, head};
  }

  /// @brief Returns a mutable range over the values in the list starting at `head`.
  [[nodiscard]] MutableInputView mutableView(std::uint32_t head) {
    return {slots_, head};
  }

  // ── Pool management ─────────────────────────────────────────────────────

  /// @brief Current pool capacity (number of slots).
  [[nodiscard]] std::uint32_t capacity() const {
    return capacity_;
  }

  /// @brief Adopt a pre-allocated buffer. Copies existing data, adds new
  /// slots to the free list, and returns the old buffer for disposal.
  /// @param newSlots newly allocated slot array (caller transfers ownership)
  /// @param newCapacity size of the new array (must be > current capacity)
  /// @return old slot array — caller must `delete[]` it (may be nullptr)
  Slot *adoptBuffer(Slot *newSlots, std::uint32_t newCapacity) {
    if (slots_) {
      std::memcpy(newSlots, slots_, capacity_ * sizeof(Slot));
    }
    for (std::uint32_t i = capacity_; i < newCapacity; i++) {
      newSlots[i].next_free = free_head_;
      free_head_ = i;
    }
    Slot *old = slots_;
    slots_ = newSlots;
    capacity_ = newCapacity;
    return old;
  }

  /// @brief Grow the pool by allocating a new buffer on the current thread.
  /// Copies existing data and adds new slots to the free list.
  /// @param newCapacity desired capacity (must be > current capacity)
  void grow(std::uint32_t newCapacity) {
    auto *newSlots = new Slot[newCapacity];
    Slot *old = adoptBuffer(newSlots, newCapacity);
    delete[] old;
  }

 private:
  Slot *slots_ = nullptr;
  std::uint32_t capacity_ = 0;
  std::uint32_t free_head_ = kNull;
};

} // namespace audioapi::utils::graph
