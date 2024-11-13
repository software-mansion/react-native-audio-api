#include "AudioArray.h"
#include "VectorMath.h"

namespace audioapi {

AudioArray::AudioArray(int size) : size_(size) {
  resize(size);
};

AudioArray::~AudioArray() {
  delete[] data_;
};

int AudioArray::getSize() const {
  return size_;
}

float& AudioArray::operator[](int index) {
  return data_[index];
}

const float& AudioArray::operator[](int index) const {
  return data_[index];
}

void AudioArray::zero() {
  memset(data_, 0, size_ * sizeof(float));
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

void AudioArray::copy(const AudioArray &source) {
  if (size_ != source.size_) {
    resize(source.size_);
  }

  memcpy(data_, source.data_, size_ * sizeof(float));
}

float AudioArray::getMaxAbsValue() const {
  return VectorMath::maximumMagnitude(data_, size_);
};

void AudioArray::normalize() {
  float maxAbsValue = getMaxAbsValue();

  if (maxAbsValue == 0.0f || maxAbsValue == 1.0f) {
    return;
  }

  VectorMath::multiplyByScalar(data_, 1.0f / maxAbsValue, data_, size_);
};

void AudioArray::scale(float value) {
  VectorMath::multiplyByScalar(data_, value, data_, size_);
};

void AudioArray::sum(const AudioArray &source) {
  VectorMath::add(data_, source.data_, data_, size_);
};

} // namespace audioapi

