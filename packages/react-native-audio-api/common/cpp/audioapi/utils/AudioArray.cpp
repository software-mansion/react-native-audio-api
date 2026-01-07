#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <algorithm>

namespace audioapi {

AudioArray::AudioArray(size_t size) : data_(nullptr), size_(size) {
  resize(size);
}

AudioArray::AudioArray(const AudioArray &other) : data_(nullptr), size_(0) {
  resize(other.size_);

  copy(&other);
}

AudioArray::AudioArray(const float *data, size_t size) : size_(size) {
  data_ = new float[size_];
  memcpy(data_, data, size_ * sizeof(float));
}

AudioArray::~AudioArray() {
  if (data_) {
    delete[] data_;
    data_ = nullptr;
  }
}

size_t AudioArray::getSize() const {
  return size_;
}

float *AudioArray::getData() const {
  return data_;
}

float &AudioArray::operator[](size_t index) {
  return data_[index];
}

const float &AudioArray::operator[](size_t index) const {
  return data_[index];
}

void AudioArray::normalize() {
  float maxAbsValue = getMaxAbsValue();

  if (maxAbsValue == 0.0f || maxAbsValue == 1.0f) {
    return;
  }

  dsp::multiplyByScalar(data_, 1.0f / maxAbsValue, data_, size_);
}

void AudioArray::resize(size_t size) {
  if (size == size_) {
    if (!data_) {
      data_ = new float[size];
    }

    zero(0, size);
    return;
  }

  delete[] data_;
  size_ = size;
  data_ = new float[size_];

  zero(0, size_);
}

void AudioArray::scale(float value) {
  dsp::multiplyByScalar(data_, value, data_, size_);
}

float AudioArray::getMaxAbsValue() const {
  return dsp::maximumMagnitude(data_, size_);
}

void AudioArray::zero() {
  zero(0, size_);
}

void AudioArray::zero(size_t start, size_t length) {
  memset(data_ + start, 0, length * sizeof(float));
}

void AudioArray::sum(const AudioArray *source) {
  sum(source, 0, 0, size_);
}

void AudioArray::sum(const AudioArray *source, size_t start, size_t length) {
  sum(source, start, start, length);
}

void AudioArray::sum(
    const AudioArray *source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  dsp::add(
      data_ + destinationStart, source->getData() + sourceStart, data_ + destinationStart, length);
}

void AudioArray::copy(const AudioArray *source) {
  copy(source, 0, size_);
}

void AudioArray::copy(const AudioArray *source, size_t start, size_t length) {
  copy(source, start, start, length);
}

void AudioArray::copy(
    const AudioArray *source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  memcpy(data_ + destinationStart, source->getData() + sourceStart, length * sizeof(float));
}

} // namespace audioapi
