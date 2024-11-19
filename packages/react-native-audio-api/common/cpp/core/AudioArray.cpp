#include <algorithm>

#include "AudioArray.h"
#include "VectorMath.h"

namespace audioapi {

AudioArray::AudioArray(int size) : size_(size) {
  resize(size);
}

AudioArray::~AudioArray() {
  delete[] data_;
}

int AudioArray::getSize() const {
  return size_;
}

float* AudioArray::getData() const {
  return data_;
}

float& AudioArray::operator[](int index) {
  return data_[index];
}

const float& AudioArray::operator[](int index) const {
  return data_[index];
}

void AudioArray::normalize() {
  float maxAbsValue = getMaxAbsValue();

  if (maxAbsValue == 0.0f || maxAbsValue == 1.0f) {
    return;
  }

  VectorMath::multiplyByScalar(data_, 1.0f / maxAbsValue, data_, size_);
}

void AudioArray::resize(int size) {
  if (size == size_) {
    zero();
    return;
  }

  delete[] data_;
  size_ = size;
  data_ = new float[size_];

  zero();
}

void AudioArray::scale(float value) {
  VectorMath::multiplyByScalar(data_, value, data_, size_);
}

float AudioArray::getMaxAbsValue() const {
  return VectorMath::maximumMagnitude(data_, size_);
}

void AudioArray::zero() {
  memset(data_, 0, size_ * sizeof(float));
}

void AudioArray::zero(int start, int length) {
  memset(data_ + start, 0, length * sizeof(float));
}

void AudioArray::sum(const AudioArray* source) {
  VectorMath::add(data_, source->getData(), data_, size_);
}

void AudioArray::sum(const AudioArray* source, int start, int length) {
  VectorMath::add(data_ + start, source->getData() + start, data_ + start, length);
}

void AudioArray::sum(const AudioArray* source, int sourceStart, int destinationStart, int length) {
  VectorMath::add(data_ + destinationStart, source->getData() + sourceStart, data_ + destinationStart, length);
}

void AudioArray::copy(const AudioArray* source) {
  memcpy(data_, source->getData(), size_ * sizeof(float));
}

void AudioArray::copy(const AudioArray* source, int start, int length) {
  memcpy(data_ + start, source->getData() + start, length * sizeof(float));
}

void AudioArray::copy(const AudioArray* source, int sourceStart, int destinationStart, int length) {
  memcpy(data_ + destinationStart, source->getData() + sourceStart, length * sizeof(float));
}

} // namespace audioapi

