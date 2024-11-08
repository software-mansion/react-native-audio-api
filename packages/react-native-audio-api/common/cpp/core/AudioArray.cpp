#include "AudioArray.h"

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

}
