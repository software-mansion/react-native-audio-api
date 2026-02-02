#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>

namespace audioapi {

class AudioArray {
 public:
  explicit AudioArray(size_t size);
  /// @note The data is copied, so it does not take ownership of the pointer
  AudioArray(const float *data, size_t size);
  ~AudioArray() = default;

  AudioArray(const AudioArray &other);
  AudioArray(AudioArray &&other) noexcept;
  AudioArray &operator=(const AudioArray &other);
  AudioArray &operator=(AudioArray &&other) noexcept;

  [[nodiscard]] inline size_t getSize() const {
    return size_;
  }

  inline float &operator[](size_t index) {
    return data_[index];
  }
  inline const float &operator[](size_t index) const {
    return data_[index];
  }

  [[nodiscard]] inline float *begin() noexcept {
    return data_.get();
  }
  [[nodiscard]] inline float *end() noexcept {
    return data_.get() + size_;
  }

  [[nodiscard]] inline const float *begin() const noexcept {
    return data_.get();
  }
  [[nodiscard]] inline const float *end() const noexcept {
    return data_.get() + size_;
  }

  void resize(size_t size);

  void zero() noexcept;
  void zero(size_t start, size_t length) noexcept;

  void sum(const AudioArray &source, float gain = 1.0f);
  void sum(
      const AudioArray &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length,
      float gain = 1.0f);

  void multiply(const AudioArray &source);
  void multiplyByScalar(float value);

  void copy(const AudioArray &source);
  void copy(const AudioArray &source, size_t sourceStart, size_t destinationStart, size_t length);

  void reverse();
  void normalize();
  void scale(float value);
  [[nodiscard]] float getMaxAbsValue() const;
  [[nodiscard]] float computeConvolution(const AudioArray &kernel, size_t startIndex = 0) const;

 private:
  std::unique_ptr<float[]> data_ = nullptr;
  size_t size_ = 0;
};

} // namespace audioapi
