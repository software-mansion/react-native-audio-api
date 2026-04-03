// InputPool out-of-line definitions (templates remain in InputPool.h).

#include <audioapi/core/utils/graph/InputPool.h>

namespace audioapi::utils::graph {

// ── Lifecycle ─────────────────────────────────────────────────────────────

InputPool::InputPool() = default;

InputPool::~InputPool() {
  delete[] slots_;
}

InputPool::InputPool(InputPool &&other) noexcept
    : slots_(other.slots_), capacity_(other.capacity_), free_head_(other.free_head_) {
  other.slots_ = nullptr;
  other.capacity_ = 0;
  other.free_head_ = kNull;
}

InputPool &InputPool::operator=(InputPool &&other) noexcept {
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

// ── Slot allocation ───────────────────────────────────────────────────────

std::uint32_t InputPool::alloc() {
  if (free_head_ == kNull) {
    grow(capacity_ == 0 ? 64 : capacity_ * 2);
  }
  std::uint32_t idx = free_head_;
  free_head_ = slots_[idx].next_free;
  return idx;
}

void InputPool::free(std::uint32_t idx) {
  slots_[idx].next_free = free_head_;
  free_head_ = idx;
}

// ── Linked-list operations ────────────────────────────────────────────────

void InputPool::push(std::uint32_t &head, std::uint32_t inputVal) {
  std::uint32_t idx = alloc();
  slots_[idx].val = inputVal;
  slots_[idx].next = head;
  head = idx;
}

bool InputPool::remove(std::uint32_t &head, std::uint32_t inputVal) {
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

void InputPool::freeAll(std::uint32_t &head) {
  while (head != kNull) {
    std::uint32_t nxt = slots_[head].next;
    free(head);
    head = nxt;
  }
}

bool InputPool::isEmpty(std::uint32_t head) {
  return head == kNull;
}

// ── Iteration ─────────────────────────────────────────────────────────────

InputPool::InputView<true> InputPool::view(std::uint32_t head) const {
  return {.slots = slots_, .head = head};
}

InputPool::InputView<false> InputPool::mutableView(std::uint32_t head) {
  return {.slots = slots_, .head = head};
}

// ── Pool management ───────────────────────────────────────────────────────

std::uint32_t InputPool::capacity() const {
  return capacity_;
}

InputPool::Slot *InputPool::adoptBuffer(Slot *newSlots, std::uint32_t newCapacity) {
  if (slots_ != nullptr) {
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

void InputPool::grow(std::uint32_t newCapacity) {
  auto *newSlots = new Slot[newCapacity];
  Slot *old = adoptBuffer(newSlots, newCapacity);
  delete[] old;
}

} // namespace audioapi::utils::graph
