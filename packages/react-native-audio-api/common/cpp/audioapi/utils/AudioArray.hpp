#pragma once

#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AlignedAllocator.hpp>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace audioapi {

/// @brief AlignedAudioArray is a simple wrapper around an aligned float vector for audio data manipulation.
/// It provides various utility functions for audio processing.
/// @tparam Alignment The memory alignment in bytes for the underlying storage.
/// @note AlignedAudioArray manages its own memory and provides copy and move semantics.
/// @note Not thread-safe.
/// @note Operations between different alignment specializations are supported.
template <size_t Alignment>
class AlignedAudioArray {
  template <size_t A>
  friend class AlignedAudioArray;

 public:
  explicit AlignedAudioArray(size_t size) : data_(size, 0.0f), size_(size) {}

  /// @brief Constructs a AlignedAudioArray from existing data.
  /// @note The data is copied, so it does not take ownership of the pointer.
  AlignedAudioArray(const float *data, size_t size) : data_(size), size_(size) {
    if (size_ > 0) {
      copy(data, 0, 0, size_);
    }
  }

  ~AlignedAudioArray() = default;

  AlignedAudioArray(const AlignedAudioArray &other) : data_(other.data_), size_(other.size_) {}

  AlignedAudioArray(AlignedAudioArray &&other) noexcept
      : data_(std::move(other.data_)), size_(std::exchange(other.size_, 0)) {}

  AlignedAudioArray &operator=(const AlignedAudioArray &other) {
    if (this != &other) {
      if (size_ != other.size_) {
        size_ = other.size_;
        data_.resize(size_);
      }
      if (size_ > 0) {
        copy(other);
      }
    }
    return *this;
  }

  AlignedAudioArray &operator=(AlignedAudioArray &&other) noexcept {
    if (this != &other) {
      data_ = std::move(other.data_);
      size_ = std::exchange(other.size_, 0);
    }
    return *this;
  }

  [[nodiscard]] size_t getSize() const noexcept {
    return size_;
  }

  float &operator[](size_t index) noexcept {
    return data_[index];
  }
  const float &operator[](size_t index) const noexcept {
    return data_[index];
  }

  [[nodiscard]] float *begin() noexcept {
    return alignedData();
  }
  [[nodiscard]] float *end() noexcept {
    return alignedData() + size_;
  }

  [[nodiscard]] const float *begin() const noexcept {
    return alignedData();
  }
  [[nodiscard]] const float *end() const noexcept {
    return alignedData() + size_;
  }

  [[nodiscard]] std::span<float> span() noexcept {
    return {alignedData(), size_};
  }

  [[nodiscard]] std::span<const float> span() const noexcept {
    return {alignedData(), size_};
  }

  [[nodiscard]] std::span<float> subSpan(size_t length, size_t offset = 0) {
    if (offset + length > size_) {
      throw std::out_of_range("AudioArray::subSpan - offset + length exceeds array size");
    }
    return {alignedData() + offset, length};
  }

  void zero() noexcept {
    zero(0, size_);
  }

  void zero(size_t start, size_t length) noexcept {
    memset(alignedData() + start, 0, length * sizeof(float));
  }

  /// @brief Sums the source array into this array with an optional gain.
  /// @note Assumes source and this are in distinct, non-overlapping memory locations.
  template <size_t OtherAlignment>
  void sum(const AlignedAudioArray<OtherAlignment> &source, float gain = 1.0f) {
    sum(source, 0, 0, size_, gain);
  }

  /// @brief Sums a sub-range of source into this array with an optional gain.
  /// @note Assumes source and this are in distinct, non-overlapping memory locations.
  template <size_t OtherAlignment>
  void sum(
      const AlignedAudioArray<OtherAlignment> &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length,
      float gain = 1.0f) {
    if (size_ - destinationStart < length || source.size_ - sourceStart < length) [[unlikely]] {
      throw std::out_of_range("Not enough data to sum two vectors.");
    }

    // Using restrict to inform the compiler that the source and destination do not overlap
    float *__restrict dest = alignedData() + destinationStart;
    const float *__restrict src = source.alignedData() + sourceStart;

    dsp::multiplyByScalarThenAddToOutput(src, gain, dest, length);
  }

  /// @brief Multiplies this array by the source array element-wise.
  /// @note Assumes source and this are in distinct, non-overlapping memory locations.
  template <size_t OtherAlignment>
  void multiply(const AlignedAudioArray<OtherAlignment> &source) {
    multiply(source, size_);
  }

  /// @brief Multiplies the first length elements of this array by the source array element-wise.
  /// @note Assumes source and this are in distinct, non-overlapping memory locations.
  template <size_t OtherAlignment>
  void multiply(const AlignedAudioArray<OtherAlignment> &source, size_t length) {
    if (size_ < length || source.size_ < length) [[unlikely]] {
      throw std::out_of_range("Not enough data to perform vector multiplication.");
    }

    float *__restrict dest = alignedData();
    const float *__restrict src = source.alignedData();

    dsp::multiply(src, dest, dest, length);
  }

  /// @brief Copies source array into this array.
  /// @note Assumes source and this are in distinct, non-overlapping memory locations.
  template <size_t OtherAlignment>
  void copy(const AlignedAudioArray<OtherAlignment> &source) { // NOLINT(build/include_what_you_use)
    copy(source, 0, 0, size_);
  }

  /// @brief Copies a sub-range of source into this array.
  /// @note Assumes source and this are in distinct, non-overlapping memory locations.
  template <size_t OtherAlignment>
  void copy(
      const AlignedAudioArray<OtherAlignment> &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length) { // NOLINT(build/include_what_you_use)
    if (source.size_ - sourceStart < length) [[unlikely]] {
      throw std::out_of_range("Not enough data to copy from source.");
    }
    copy(source.alignedData(), sourceStart, destinationStart, length);
  }

  /// @brief Copies data from a raw float pointer into this array.
  /// @note Assumes source and this are in distinct, non-overlapping memory locations.
  void copy(
      const float *source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length) { // NOLINT(build/include_what_you_use)
    if (size_ - destinationStart < length) [[unlikely]] {
      throw std::out_of_range("Not enough space to copy to destination.");
    }
    memcpy(alignedData() + destinationStart, source + sourceStart, length * sizeof(float));
  }

  /// @brief Copies source array in reverse order into this array.
  /// @note Assumes source and this are in distinct, non-overlapping memory locations.
  /// @note Precondition: length > 0. Reads source[sourceStart .. sourceStart-length+1].
  template <size_t OtherAlignment>
  void copyReverse(
      const AlignedAudioArray<OtherAlignment> &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length) {
    // Reverse read walks from sourceStart down to sourceStart-length+1, so the source
    // precondition is sourceStart < source.size_ on the high end and sourceStart+1 >= length
    // on the low end. Caller must also pass length > 0.
    if (size_ - destinationStart < length || sourceStart + 1 < length ||
        sourceStart >= source.size_) [[unlikely]] {
      throw std::out_of_range("Not enough space to copy to destination or from source.");
    }

    auto dstView = this->subSpan(length, destinationStart);
    auto srcView = source.span();
    const float *__restrict srcPtr = &srcView[sourceStart];

    for (size_t i = 0; i < length; ++i) {
      dstView[i] = srcPtr[-static_cast<ptrdiff_t>(i)];
    }
  }

  /// @brief Copies data from this array to a raw float pointer.
  /// @note Assumes destination and this are in distinct, non-overlapping memory locations.
  void copyTo(float *destination, size_t sourceStart, size_t destinationStart, size_t length)
      const {
    if (size_ - sourceStart < length) [[unlikely]] {
      throw std::out_of_range("Not enough data to copy from source.");
    }
    memcpy(destination + destinationStart, alignedData() + sourceStart, length * sizeof(float));
  }

  /// @brief Copies a sub-section of the array to another location within itself.
  void copyWithin(size_t sourceStart, size_t destinationStart, size_t length) {
    if (size_ - sourceStart < length || size_ - destinationStart < length) [[unlikely]] {
      throw std::out_of_range("Not enough space for moving data or data to move.");
    }
    float *base = alignedData();
    memmove(base + destinationStart, base + sourceStart, length * sizeof(float));
  }

  void reverse() {
    if (size_ <= 1) {
      return;
    }
    std::reverse(begin(), end());
  }

  void normalize() {
    float maxAbsValue = getMaxAbsValue();
    if (maxAbsValue == 0.0f || maxAbsValue == 1.0f) {
      return;
    }
    float *data = alignedData();
    dsp::multiplyByScalar(data, 1.0f / maxAbsValue, data, size_);
  }

  void scale(float value) {
    float *data = alignedData();
    dsp::multiplyByScalar(data, value, data, size_);
  }

  [[nodiscard]] float getMaxAbsValue() const {
    return dsp::maximumMagnitude(alignedData(), size_);
  }

 private:
  [[nodiscard]] float *alignedData() noexcept {
    return std::assume_aligned<Alignment>(data_.data());
  }
  [[nodiscard]] const float *alignedData() const noexcept {
    return std::assume_aligned<Alignment>(data_.data());
  }

 protected:
  std::vector<float, AlignedAllocator<float, Alignment>> data_;
  size_t size_ = 0;
};

inline constexpr size_t DSP_ALIGNMENT = 64;
using AudioArray = AlignedAudioArray<alignof(std::max_align_t)>;
using DSPAudioArray = AlignedAudioArray<DSP_ALIGNMENT>;

} // namespace audioapi
