#include <audioapi/utils/CircularAudioArray.h>

#include <algorithm>

namespace audioapi {

CircularAudioArray::CircularAudioArray(size_t size) : AudioArray(size) {}

void CircularAudioArray::push_back(const AudioArray & data, size_t size, bool skipAvailableSpaceCheck) {
    push_back(data.begin(), size, skipAvailableSpaceCheck);
}

void CircularAudioArray::push_back(const float *data, size_t size, bool skipAvailableSpaceCheck) {
    if (size > size_) {
        throw std::overflow_error("size exceeds CircularAudioArray size_");
    }

    if (size > getAvailableSpace() && !skipAvailableSpaceCheck) {
        throw std::overflow_error("not enough space in CircularAudioArray");
    }

    if (vWriteIndex_ + size > size_) {
        auto partSize = size_ - vWriteIndex_;
        copy(data, 0, vWriteIndex_, partSize);
        copy(data, partSize, 0, size - partSize);
    } else {
        copy(data, 0, vWriteIndex_, size);
    }

    vWriteIndex_ = vWriteIndex_ + size > size_ ? vWriteIndex_ + size - size_ : vWriteIndex_ + size;
}

void CircularAudioArray::pop_front(AudioArray &data, size_t size, bool skipAvailableDataCheck) {
  pop_front(data.begin(), size, skipAvailableDataCheck);
}

void CircularAudioArray::pop_front(float *data, size_t size, bool skipAvailableDataCheck) {
    if (size > size_) {
        throw std::overflow_error("size exceeds CircularAudioArray size_");
    }

    if (size > getNumberOfAvailableFrames() && !skipAvailableDataCheck) {
        throw std::overflow_error("not enough data in CircularAudioArray");
    }

    if (vReadIndex_ + size > size_) {
        auto partSize = size_ - vReadIndex_;
        copyTo(data, vReadIndex_, 0, partSize);
        copyTo(data, 0, partSize, size - partSize);
    } else {
        copyTo(data, vReadIndex_, 0, size);
    }

    vReadIndex_ = vReadIndex_ + size > size_ ? vReadIndex_ + size - size_ : vReadIndex_ + size;
}

void CircularAudioArray::pop_back(
    AudioArray &data,
    size_t size,
    size_t offset,
    bool skipAvailableDataCheck) {
    pop_back(data.begin(), size, offset, skipAvailableDataCheck);
}

void CircularAudioArray::pop_back(float *data, size_t size, size_t offset,
                                      bool skipAvailableDataCheck) {
    if (size > size_) {
        throw std::overflow_error("size exceeds CircularAudioArray size_");
    }

    if (size + offset > getNumberOfAvailableFrames() && !skipAvailableDataCheck) {
        throw std::overflow_error("not enough data in CircularAudioArray");
    }

    if (vWriteIndex_ <= offset) {
        copyTo(data, size_ - (offset - vWriteIndex_) - size, 0, size);
    } else if (vWriteIndex_ <= size + offset) {
        auto partSize = size + offset - vWriteIndex_;
        copyTo(data, size_ - partSize, 0, partSize);
        copyTo(data, 0, partSize, size - partSize);
    } else {
        copyTo(data, vWriteIndex_ - size - offset, 0, size);
    }

    vReadIndex_ = vWriteIndex_ - offset < 0 ? size + vWriteIndex_ - offset : vWriteIndex_ - offset;
}

size_t CircularAudioArray::getNumberOfAvailableFrames() const {
  return vWriteIndex_ >= vReadIndex_ ? vWriteIndex_ - vReadIndex_
                                     : size_ - vReadIndex_ + vWriteIndex_;
}

size_t CircularAudioArray::getAvailableSpace() const {
  return size_ - getNumberOfAvailableFrames();
}

} // namespace audioapi
