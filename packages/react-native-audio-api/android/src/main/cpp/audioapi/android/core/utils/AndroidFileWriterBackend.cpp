#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <audioapi/libs/miniaudio/miniaudio.h>

#include <cstddef>
#include <cstring>
#include <memory>
#include <new>

namespace audioapi {
AndroidFileWriterBackend::AndroidFileWriterBackend(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties)
    : AudioFileWriter(audioEventHandlerRegistry, fileProperties) {}

void AndroidFileWriterBackend::createOffloader() {
  auto offloaderLambda = [this](WriterData data) {
    runWriterTask(data);
  };
  offloader_ = std::make_unique<task_offloader::TaskOffloader<
      WriterData,
      FILE_WRITER_SPSC_OVERFLOW_STRATEGY,
      FILE_WRITER_SPSC_WAIT_STRATEGY>>(FILE_WRITER_CHANNEL_CAPACITY, offloaderLambda);
}

bool AndroidFileWriterBackend::initializePreallocatedInputPool() {
  cleanupPreallocatedInputPool();

  if (streamMaxBufferSize_ <= 0 || streamChannelCount_ <= 0) {
    return false;
  }

  inputBufferBytesPerSlot_ = static_cast<size_t>(streamMaxBufferSize_) *
      static_cast<size_t>(streamChannelCount_) * ma_get_bytes_per_sample(ma_format_f32);
  size_t inputPoolBytes = inputBufferBytesPerSlot_ * FILE_WRITER_POOL_SIZE;
  // nothrow new keeps the graceful failure path (return false) instead of throwing.
  inputBufferPool_.reset(new (std::nothrow) std::byte[inputPoolBytes]);
  if (inputBufferPool_ == nullptr) {
    return false;
  }

  inputBuffers_.clear();
  inputBuffers_.reserve(FILE_WRITER_POOL_SIZE);
  std::byte *poolHead = inputBufferPool_.get();
  for (size_t i = 0; i < FILE_WRITER_POOL_SIZE; ++i) {
    inputBuffers_.push_back(poolHead + i * inputBufferBytesPerSlot_);
  }

  freeSlots_ = std::make_unique<FreeList>();
  freeSlots_->seed();

  // Recreated on every open (closeFile tears it down), last so it never sees a half-built pool.
  createOffloader();
  return true;
}

void AndroidFileWriterBackend::cleanupPreallocatedInputPool() {
  // Stop the worker before freeing the pool/free list it accesses.
  offloader_.reset();
  freeSlots_.reset();
  inputBuffers_.clear();
  inputBufferPool_.reset();
  inputBufferBytesPerSlot_ = 0;
}

void AndroidFileWriterBackend::writeAudioData(void *data, int numFrames) {
  if (offloader_ == nullptr || freeSlots_ == nullptr || inputBufferBytesPerSlot_ == 0 ||
      inputBuffers_.empty()) {
    return;
  }

  auto slot = freeSlots_->tryAcquire();
  if (!slot.has_value()) {
    return;
  }

  size_t bytes = static_cast<size_t>(numFrames) * static_cast<size_t>(streamChannelCount_) *
      ma_get_bytes_per_sample(ma_format_f32);
  if (bytes > inputBufferBytesPerSlot_) {
    freeSlots_->release(slot.value());
    return;
  }

  // Oboe owns `data` only for the duration of this synchronous
  // callback. Copy into an owned buffer before handing off to the
  // worker thread; the consumer in runWriterTask releases the slot.
  std::memcpy(inputBuffers_[slot.value()], data, bytes);
  WriterData writerData{.slot = slot.value(), .numFrames = numFrames};
  // send() cannot block here: we hold a slot from a pool of FILE_WRITER_POOL_SIZE,
  // and the channel is sized one larger, so the ring always has room while any slot
  // is in flight.
  offloader_->getSender()->send(writerData);
}

void AndroidFileWriterBackend::runWriterTask(WriterData data) {
  auto [slot, numFrames] = data;
  if (slot == FreeList::kSentinel) {
    return;
  }
  if (slot >= inputBuffers_.size() || freeSlots_ == nullptr) {
    return;
  }

  processWriterData(inputBuffers_[slot], numFrames);
  freeSlots_->release(slot);
}

} // namespace audioapi
