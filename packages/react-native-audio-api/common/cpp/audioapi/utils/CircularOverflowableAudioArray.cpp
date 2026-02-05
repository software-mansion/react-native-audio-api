#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <algorithm>
#include <type_traits>

namespace audioapi {

CircularOverflowableAudioArray::CircularOverflowableAudioArray(size_t size) noexcept(
    std::is_nothrow_constructible<AudioArray, size_t>::value)
    : AudioArray(size) {}

void CircularOverflowableAudioArray::write(const AudioArray &data, const size_t size) {
  write(data.begin(), size);
}

void CircularOverflowableAudioArray::write(const float *data, size_t size) {
    size_t writeIndex = vWriteIndex_.load(std::memory_order_relaxed);

    if (size > size_) {
        return; // Ignore write if size exceeds buffer size
    }

    /// Advances the read index if there is not enough space
    readLock_.lock();
    size_t availableSpace = (size_ + vReadIndex_ - writeIndex - 1) % size_;
    if (size > availableSpace) {
        vReadIndex_ = (writeIndex + size + 1) % size_;
    }
    readLock_.unlock();

    size_t partSize = size_ - writeIndex;
    if (size > partSize) {
        copy(data, 0, writeIndex, partSize);
        copy(data, partSize, 0, size - partSize);
    } else {
        copy(data, 0, writeIndex, size);
    }
    vWriteIndex_.store((writeIndex + size) % size_, std::memory_order_relaxed);
}

size_t CircularOverflowableAudioArray::read(AudioArray &data, size_t size) const {
  return read(data.begin(), size);
}

size_t CircularOverflowableAudioArray::read(float *data, size_t size) const {
    readLock_.lock();
    size_t availableSpace = getAvailableSpace();
    size_t readSize = std::min(size, availableSpace);

    size_t partSize = size_ - vReadIndex_;
    if (readSize > partSize) {
        copyTo(data, vReadIndex_, 0, partSize);
        copyTo(data, 0, partSize, readSize - partSize);
    } else {
        copyTo(data, vReadIndex_, 0, readSize);
    }
    vReadIndex_ = (vReadIndex_ + readSize) % size_;
    readLock_.unlock();
    return readSize;
}

size_t CircularOverflowableAudioArray::getAvailableSpace() const {
  return (size_ + vWriteIndex_.load(std::memory_order_relaxed) - vReadIndex_) % size_;
}

} // namespace audioapi
