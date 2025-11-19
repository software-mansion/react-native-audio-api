#include <audioapi/utils/CircularAudioArray.h>

namespace audioapi {

CircularAudioArray::CircularAudioArray(size_t size) : AudioArray(size) {}

void CircularAudioArray::push_back(const float *data, size_t size, bool skipAvailableSpaceCheck) {
  if (size > size_) {
    throw std::overflow_error("size exceeds CircularAudioArray size_");
  }

  if (size > getAvailableSpace() && !skipAvailableSpaceCheck) {
    throw std::overflow_error("not enough space in CircularAudioArray");
  }

  if (vWriteIndex_ + size > size_) {
    auto partSize = size_ - vWriteIndex_;
    memcpy(data_ + vWriteIndex_, data, partSize * sizeof(float));
    memcpy(data_, data + partSize, (size - partSize) * sizeof(float));
  } else {
    memcpy(data_ + vWriteIndex_, data, size * sizeof(float));
  }

  vWriteIndex_ = vWriteIndex_ + size > size_ ? vWriteIndex_ + size - size_ : vWriteIndex_ + size;
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
    memcpy(data, data_ + vReadIndex_, partSize * sizeof(float));
    memcpy(data + partSize, data_, (size - partSize) * sizeof(float));
  } else {
    memcpy(data, data_ + vReadIndex_, size * sizeof(float));
  }

  vReadIndex_ = vReadIndex_ + size > size_ ? vReadIndex_ + size - size_ : vReadIndex_ + size;
}

void CircularAudioArray::pop_back(
    float *data,
    size_t size,
    size_t offset,
    bool skipAvailableDataCheck) {
  if (size > size_) {
    throw std::overflow_error("size exceeds CircularAudioArray size_");
  }

  if (size + offset > getNumberOfAvailableFrames() && !skipAvailableDataCheck) {
    throw std::overflow_error("not enough data in CircularAudioArray");
  }

  if (vWriteIndex_ <= offset) {
    memcpy(data, data_ + size_ - (offset - vWriteIndex_) - size, size * sizeof(float));
  } else if (vWriteIndex_ <= size + offset) {
    auto partSize = size + offset - vWriteIndex_;
    memcpy(data, data_ + size_ - partSize, partSize * sizeof(float));
    memcpy(data + partSize, data_, (size - partSize) * sizeof(float));
  } else {
    memcpy(data, data_ + vWriteIndex_ - size - offset, size * sizeof(float));
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
