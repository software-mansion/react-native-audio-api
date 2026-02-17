#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace audioapi {

AudioArray::AudioArray(size_t size) : size_(size) {
  if (size_ > 0) {
    data_ = std::make_unique<float[]>(size_);
    zero();
  }
}

AudioArray::AudioArray(const float *data, size_t size) : size_(size) {
  if (size_ > 0) {
    data_ = std::make_unique<float[]>(size_);
    copy(data, 0, 0, size_);
  }
}

AudioArray::AudioArray(const AudioArray &other) : size_(other.size_) {
  if (size_ > 0 && other.data_) {
    data_ = std::make_unique<float[]>(size_);
    copy(other);
  }
}

AudioArray::AudioArray(audioapi::AudioArray &&other) noexcept
    : data_(std::move(other.data_)), size_(std::exchange(other.size_, 0)) {}

AudioArray &AudioArray::operator=(const audioapi::AudioArray &other) {
  if (this != &other) {
    if (size_ != other.size_) {
      size_ = other.size_;
      data_ = (size_ > 0) ? std::make_unique<float[]>(size_) : nullptr;
    }

    if (size_ > 0 && data_) {
      copy(other);
    }
  }

  return *this;
}

AudioArray &AudioArray::operator=(audioapi::AudioArray &&other) noexcept {
  if (this != &other) {
    data_ = std::move(other.data_);
    size_ = std::exchange(other.size_, 0);
  }

  return *this;
}

void AudioArray::zero() noexcept {
  zero(0, size_);
}

void AudioArray::zero(size_t start, size_t length) noexcept {
  memset(data_.get() + start, 0, length * sizeof(float));
}

void AudioArray::sum(const AudioArray &source, float gain) {
  sum(source, 0, 0, size_, gain);
}

void AudioArray::sum(
    const AudioArray &source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length,
    float gain) {
  if (size_ - destinationStart < length || source.size_ - sourceStart < length) [[unlikely]] {
    throw std::out_of_range("Not enough data to sum two vectors.");
  }

  // Using restrict to inform the compiler that the source and destination do not overlap
  float *__restrict dest = data_.get() + destinationStart;
  const float *__restrict src = source.data_.get() + sourceStart;

  dsp::multiplyByScalarThenAddToOutput(src, gain, dest, length);
}

void AudioArray::multiply(const AudioArray &source) {
  multiply(source, size_);
}

void AudioArray::multiply(const audioapi::AudioArray &source, size_t length) {
  if (size_ < length || source.size_ < length) [[unlikely]] {
    throw std::out_of_range("Not enough data to perform vector multiplication.");
  }

  float *__restrict dest = data_.get();
  const float *__restrict src = source.data_.get();

  dsp::multiply(src, dest, dest, length);
}

void AudioArray::copy(const AudioArray &source) {
  copy(source, 0, 0, size_);
}

void AudioArray::copy(
    const AudioArray &source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  if (source.size_ - sourceStart < length) [[unlikely]] {
    throw std::out_of_range("Not enough data to copy from source.");
  }

  copy(source.data_.get(), sourceStart, destinationStart, length);
}

void AudioArray::copy(
    const float *source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  if (size_ - destinationStart < length) [[unlikely]] {
    throw std::out_of_range("Not enough space to copy to destination.");
  }

  memcpy(data_.get() + destinationStart, source + sourceStart, length * sizeof(float));
}

void AudioArray::copyReverse(
        const audioapi::AudioArray &source,
        size_t sourceStart,
        size_t destinationStart,
        size_t length) {
    if (size_ - destinationStart < length || source.size_ - sourceStart < length) [[unlikely]] {
        throw std::out_of_range("Not enough space to copy to destination or from source.");
    }

    auto dstView = this->subSpan(length, destinationStart);
    auto srcView = source.span();
    const float *__restrict srcPtr = &srcView[sourceStart];

    for (size_t i = 0; i < length; ++i) {
        dstView[i] = srcPtr[-static_cast<ptrdiff_t>(i)];
    }
}

void AudioArray::copyTo(
    float *destination,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) const {
  if (size_ - sourceStart < length) [[unlikely]] {
    throw std::out_of_range("Not enough data to copy from source.");
  }

  memcpy(destination + destinationStart, data_.get() + sourceStart, length * sizeof(float));
}

void AudioArray::copyWithin(size_t sourceStart, size_t destinationStart, size_t length) {
  if (size_ - sourceStart < length || size_ - destinationStart < length) [[unlikely]] {
    throw std::out_of_range("Not enough space for moving data or data to move.");
  }

  memmove(data_.get() + destinationStart, data_.get() + sourceStart, length * sizeof(float));
}

void AudioArray::reverse() {
  if (size_ <= 1) {
    return;
  }

  std::reverse(begin(), end());
}

void AudioArray::normalize() {
  float maxAbsValue = getMaxAbsValue();

  if (maxAbsValue == 0.0f || maxAbsValue == 1.0f) {
    return;
  }

  dsp::multiplyByScalar(data_.get(), 1.0f / maxAbsValue, data_.get(), size_);
}

void AudioArray::scale(float value) {
  dsp::multiplyByScalar(data_.get(), value, data_.get(), size_);
}

float AudioArray::getMaxAbsValue() const {
  return dsp::maximumMagnitude(data_.get(), size_);
}

float AudioArray::computeConvolution(const audioapi::AudioArray &kernel, size_t startIndex) const {
  if (kernel.size_ > size_ - startIndex) [[unlikely]] {
    throw std::out_of_range("Kernal size exceeds available data for convolution.");
  }

  return dsp::computeConvolution(data_.get() + startIndex, kernel.data_.get(), kernel.size_);
}

} // namespace audioapi
