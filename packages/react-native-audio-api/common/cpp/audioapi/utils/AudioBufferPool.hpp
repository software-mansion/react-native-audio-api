#pragma once

#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/SlotFreeList.hpp>

#include <cstddef>
#include <memory>
#include <new>
#include <vector>

namespace audioapi {

class AudioBufferReturner;

/// Non-template face of AudioBufferPool, so a lease can name its pool without knowing the
/// pool's capacity.
class AudioBufferPoolBase {
 public:
  AudioBufferPoolBase() = default;
  virtual ~AudioBufferPoolBase() = default;
  DELETE_COPY_AND_MOVE(AudioBufferPoolBase);

 protected:
  friend class AudioBufferReturner;
  virtual void release(AudioBuffer *buffer) noexcept = 0;
};

/// unique_ptr deleter that hands a leased buffer back to its pool instead of freeing it.
class AudioBufferReturner {
 public:
  AudioBufferReturner() = default;
  explicit AudioBufferReturner(AudioBufferPoolBase *pool) : pool_(pool) {}

  void operator()(AudioBuffer *buffer) const noexcept {
    if (pool_ != nullptr) {
      pool_->release(buffer);
    }
  }

 private:
  AudioBufferPoolBase *pool_ = nullptr;
};

/// A buffer on loan from an AudioBufferPool. Dropping it returns the buffer; the pool keeps
/// ownership of the memory throughout. Null when no buffer is held.
using AudioBufferLease = std::unique_ptr<AudioBuffer, AudioBufferReturner>;

/// Fixed set of preallocated planar buffers handed out as leases. Acquire and return are
/// lock-free, allocation-free and safe from any thread (see SlotFreeList), so the audio
/// thread may acquire and a worker may return. The pool must outlive every lease it hands out.
template <size_t Capacity>
class AudioBufferPool final : public AudioBufferPoolBase {
 public:
  /// Allocates every buffer and marks it free. JS thread, with no lease outstanding. Returns
  /// false, leaving the pool empty, when memory runs out.
  bool allocate(size_t frames, int channelCount, float sampleRate) {
    clear();
    try {
      buffers_.reserve(Capacity);
      for (size_t i = 0; i < Capacity; ++i) {
        buffers_.emplace_back(frames, channelCount, sampleRate);
      }
    } catch (const std::bad_alloc &) {
      buffers_.clear();
      return false;
    }
    freeSlots_.seed();
    return true;
  }

  /// Frees every buffer. Only once no lease is outstanding.
  void clear() {
    buffers_.clear();
  }

  [[nodiscard]] bool isAllocated() const {
    return !buffers_.empty();
  }

  /// A lease on a free buffer, or a null lease when none is left or the pool is not allocated.
  [[nodiscard]] AudioBufferLease tryAcquire() {
    if (buffers_.empty()) {
      return {};
    }
    const auto slot = freeSlots_.tryAcquire();
    if (!slot.has_value()) {
      return {};
    }
    return AudioBufferLease(&buffers_[slot.value()], AudioBufferReturner(this));
  }

 protected:
  /// Never blocks. Foreign or null pointers are ignored, and a double return is a no-op.
  void release(AudioBuffer *buffer) noexcept override {
    if (buffer == nullptr || buffers_.empty()) {
      return;
    }
    const std::ptrdiff_t index = buffer - buffers_.data();
    if (index < 0 || static_cast<size_t>(index) >= buffers_.size()) {
      return;
    }
    freeSlots_.release(static_cast<size_t>(index));
  }

 private:
  std::vector<AudioBuffer> buffers_;
  slots::SlotFreeList<Capacity> freeSlots_;
};

} // namespace audioapi
