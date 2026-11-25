#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioBufferPool.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace audioapi {

/// A filled pool buffer and how many of its frames count: the only thing that crosses from the
/// audio thread to the worker. The default-constructed value (no buffer) is the offloader's
/// shutdown message, which is what operator== exists for.
struct PendingFrames {
  AudioBufferLease buffer;
  size_t numFrames = 0;

  bool operator==(const PendingFrames &) const = default;
};

/// @brief Carries audio-thread buffers to a worker thread: the audio thread copies each one into a
/// preallocated pool slot and hands the lease to @p deliver on the worker, which returns the slot
/// by dropping the lease.
///
/// start() and stop() run on the owner's control thread, never while push() may run: the owner
/// stops feeding push() first. push() never blocks or allocates.
class AudioBufferOffloader final {
 public:
  using Deliver = std::function<void(PendingFrames)>;

  explicit AudioBufferOffloader(Deliver deliver) : deliver_(std::move(deliver)) {}
  DELETE_COPY_AND_MOVE(AudioBufferOffloader);
  ~AudioBufferOffloader() {
    stop();
  }

  /// Sizes the pool for @p format and starts the worker. Returns false, with nothing running, on
  /// an unusable format or when memory runs out.
  bool start(const StreamFormat &format) {
    stop();
    const auto &layout = format.layout;
    if (format.maxFramesPerBuffer == 0 || layout.channelCount <= 0 ||
        layout.channelCount > MAX_CHANNEL_COUNT) {
      return false;
    }
    if (!pool_.allocate(format.maxFramesPerBuffer, layout.channelCount, layout.sampleRate)) {
      return false;
    }
    // Last, so the worker never sees a half-built pool.
    worker_ = std::make_unique<Offloader>(
        CHANNEL_CAPACITY, [this](PendingFrames pending) { deliver_(std::move(pending)); });
    return true;
  }

  /// Hands every queued buffer to deliver, joins the worker, then frees the pool.
  void stop() {
    worker_.reset();
    pool_.clear();
  }

  /// Copies the first @p numFrames frames of @p source into a pool slot and hands it to the
  /// worker. @p source is anything AudioBuffer::copy() takes: raw channel pointers, one per channel
  /// of the started format (what the audio thread has), or another buffer, mixed to the started
  /// channel count. Never blocks or allocates. Dropped when stopped, when no pool slot is free, or
  /// when longer than the format's maxFramesPerBuffer.
  template <AudioBufferCopySource Source>
  void push(const Source &source, size_t numFrames) {
    if constexpr (std::is_pointer_v<Source>) {
      if (source == nullptr) {
        return;
      }
    }
    if (worker_ == nullptr || numFrames == 0) {
      return;
    }
    AudioBufferLease slot = pool_.tryAcquire();
    if (slot == nullptr || numFrames > slot->getSize()) {
      return;
    }
    slot->copy(source, 0, 0, numFrames);
    worker_->getSender()->send(PendingFrames{.buffer = std::move(slot), .numFrames = numFrames});
  }

 private:
  static constexpr size_t POOL_SIZE = 32;
  // At most POOL_SIZE buffers are in flight and an SPSC ring holds (capacity - 1), so send() on
  // the audio thread always finds room and never waits.
  static constexpr size_t CHANNEL_CAPACITY = POOL_SIZE + 1;

  using Offloader = task_offloader::TaskOffloader<
      PendingFrames,
      channels::spsc::OverflowStrategy::OVERWRITE_ON_FULL,
      channels::spsc::WaitStrategy::ATOMIC_WAIT>;

  Deliver deliver_;
  /// Declared before worker_, so the worker is joined before the pool it reads from is freed.
  AudioBufferPool<POOL_SIZE> pool_;
  std::unique_ptr<Offloader> worker_;
};

} // namespace audioapi
