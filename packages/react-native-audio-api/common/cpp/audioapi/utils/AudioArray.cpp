#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>

#include <algorithm>
#include <utility>
#include <memory>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

namespace audioapi {

AudioArray::AudioArray(size_t size): size_(size) {
    if (size_ > 0) {
        data_ = std::make_unique<float[]>(size_);
        zero();
    }
}

AudioArray::AudioArray(const float *data, size_t size) : size_(size) {
    if (size_ > 0) {
        data_ = std::make_unique<float[]>(size_);
        std::memcpy(data_.get(), data, size_ * sizeof(float));
    }
}

AudioArray::AudioArray(const AudioArray &other) : size_(other.size_) {
    if (size_ > 0 && other.data_) {
        data_ = std::make_unique<float[]>(size_);
        std::memcpy(data_.get(), other.data_.get(), size_ * sizeof(float));
    }
}

AudioArray::AudioArray(audioapi::AudioArray &&other) noexcept : data_(std::move(other.data_)), size_(other.size_) {
  other.size_ = 0;
}

AudioArray &AudioArray::operator=(const audioapi::AudioArray &other) {
    if (this != &other) {
      if (size_ != other.size_) {
        size_ = other.size_;
        data_ = (size_ > 0) ? std::make_unique<float[]>(size_) : nullptr;
      }

      if (size_ > 0 && data_) {
      std::memcpy(data_.get(), other.data_.get(), size_ * sizeof(float));
      }
    }

    return *this;
}

AudioArray &AudioArray::operator=(audioapi::AudioArray &&other) noexcept {
    if (this != &other) {
        data_ = std::move(other.data_);
        size_ = other.size_;
        other.size_ = 0;
    }

    return *this;
}

void AudioArray::resize(size_t size) {
  if (size == size_ && data_ != nullptr) {
    zero();
    return;
  }

  size_ = size;
  data_ = (size_ > 0) ? std::make_unique<float[]>(size_) : nullptr;
  if (data_ != nullptr) {
    zero();
  }
}

void AudioArray::zero() noexcept {
  zero(0, size_);
}

void AudioArray::zero(size_t start, size_t length) noexcept {
    if (data_ == nullptr || length <= 0) {
      return;
    }

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
  if (length == 0 || data_ == nullptr || source.data_ == nullptr) {
    return;
  }

  // Using restrict to inform the compiler that the source and destination do not overlap
  float* __restrict dest = data_.get() + destinationStart;
  const float* __restrict src = source.data_.get() + sourceStart;

  dsp::multiplyByScalarThenAddToOutput(src, gain, dest, length);
}

void AudioArray::multiply(const AudioArray &source) {
  multiply(source, size_);
}

void AudioArray::multiply(const audioapi::AudioArray &source, size_t length) {
  if (data_ == nullptr || source.data_ == nullptr) {
    return;
  }

  float* __restrict dest = data_.get();
  const float* __restrict src = source.data_.get();

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
    if (length == 0 || data_ == nullptr || source.data_ == nullptr) {
        return;
    }

  memcpy(data_.get() + destinationStart, source.data_.get() + sourceStart, length * sizeof(float));
}

void AudioArray::reverse() {
    if (data_ == nullptr && size_ > 1) {
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
    if (data_ == nullptr) {
        return;
    }

    dsp::multiplyByScalar(data_.get(), value, data_.get(), size_);
}

float AudioArray::getMaxAbsValue() const {
    if (data_ == nullptr) {
        return 0.0f;
    }

    return dsp::maximumMagnitude(data_.get(), size_);
}

float AudioArray::computeConvolution(const audioapi::AudioArray &kernel, size_t startIndex) const {
    const auto kernelSize = kernel.size_;

    if (startIndex + kernelSize > size_ || !data_ || !kernel.data_) {
        return 0.0f;
    }

    const auto stateStart = data_.get() + startIndex;
    const auto kernelStart = kernel.data_.get();

    float sum = 0.0f;
    size_t k = 0;

#ifdef __ARM_NEON
    float32x4_t vSum = vdupq_n_f32(0.0f);

  // process 4 samples at a time
  for (; k <= kernelSize_ - 4; k += 4) {
    float32x4_t vState = vld1q_f32(stateStart + k);
    float32x4_t vKernel = vld1q_f32(kernelStart + k);

    // fused multiply-add: vSum += vState * vKernel
    vSum = vmlaq_f32(vSum, vState, vKernel);
  }

  // horizontal reduction: Sum the 4 lanes of vSum into a single float
  sum += vgetq_lane_f32(vSum, 0);
  sum += vgetq_lane_f32(vSum, 1);
  sum += vgetq_lane_f32(vSum, 2);
  sum += vgetq_lane_f32(vSum, 3);
#endif

    for (; k < kernelSize; ++k) {
        sum += stateStart[k] * kernelStart[k];
    }

    return sum;
}

} // namespace audioapi
