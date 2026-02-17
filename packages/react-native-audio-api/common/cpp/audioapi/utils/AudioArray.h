#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <span>

namespace audioapi {

/// @brief AudioArray is a simple wrapper around a float array for audio data manipulation.
/// It provides various utility functions for audio processing.
/// @note AudioArray manages its own memory and provides copy and move semantics.
/// @note Not thread-safe.
class AudioArray {
 public:
  explicit AudioArray(size_t size);

  /// @brief Constructs an AudioArray from existing data.
  /// @note The data is copied, so it does not take ownership of the pointer
  AudioArray(const float *data, size_t size);
  ~AudioArray() = default;

  AudioArray(const AudioArray &other);
  AudioArray(AudioArray &&other) noexcept;
  AudioArray &operator=(const AudioArray &other);
  AudioArray &operator=(AudioArray &&other) noexcept;

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
    return data_.get();
  }
  [[nodiscard]] float *end() noexcept {
    return data_.get() + size_;
  }

  [[nodiscard]] const float *begin() const noexcept {
    return data_.get();
  }
  [[nodiscard]] const float *end() const noexcept {
    return data_.get() + size_;
  }

  [[nodiscard]] std::span<float> span() noexcept {
    return {data_.get(), size_};
  }

  [[nodiscard]] std::span<const float> span() const noexcept {
    return {data_.get(), size_};
  }

  [[nodiscard]] std::span<float> subSpan(size_t length, size_t offset = 0) {
    if (offset + length > size_) {
      throw std::out_of_range("AudioArray::subSpan - offset + length exceeds array size");
    }

    return {data_.get() + offset, length};
  }

  void zero() noexcept;
  void zero(size_t start, size_t length) noexcept;

  /// @brief Sums the source AudioArray into this AudioArray with an optional gain.
  /// @param source The source AudioArray to sum from.
  /// @param gain The gain to apply to the source before summing. Default is 1.0f.
  /// @note Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void sum(const AudioArray &source, float gain = 1.0f);

  /// @brief Sums the source AudioArray into this AudioArray with an optional gain.
  /// @param source The source AudioArray to sum from.
  /// @param sourceStart The starting index in the source AudioArray.
  /// @param destinationStart The starting index in this AudioArray.
  /// @param length The number of samples to sum.
  /// @param gain The gain to apply to the source before summing. Default is 1.0f.
  /// @note Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void sum(
      const AudioArray &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length,
      float gain = 1.0f);

  /// @brief Multiplies this AudioArray by the source AudioArray element-wise.
  /// @param source The source AudioArray to multiply with.
  /// @note Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void multiply(const AudioArray &source);

  /// @brief Multiplies this AudioArray by the source AudioArray element-wise.
  /// @param source The source AudioArray to multiply with.
  /// @param length The number of samples to multiply.
  /// @note Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void multiply(const AudioArray &source, size_t length);

  /// @brief Copies source AudioArray into this AudioArray
  /// @param source The source AudioArray to copy.
  /// @note Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void copy(const AudioArray &source);

  /// @brief Copies source AudioArray into this AudioArray
  /// @param source The source AudioArray to copy.
  /// @param sourceStart The starting index in the source AudioArray.
  /// @param destinationStart The starting index in this AudioArray.
  /// @param length The number of samples to copy.
  /// @note Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void copy(const AudioArray &source, size_t sourceStart, size_t destinationStart, size_t length);

  /// @brief Copies data from a raw float pointer into this AudioArray.
  /// @param source The source float pointer to copy from.
  /// @param sourceStart The starting index in the source float pointer.
  /// @param destinationStart The starting index in this AudioArray.
  /// @param length The number of samples to copy.
  /// @note Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void copy(const float *source, size_t sourceStart, size_t destinationStart, size_t length);

  /// @brief Copies data from the source AudioArray in reverse order into this AudioArray.
  /// @param source The source AudioArray to copy from.
  /// @param sourceStart The starting index in the source AudioArray.
  /// @param destinationStart The starting index in this AudioArray.
  /// @param length The number of samples to copy.
  /// @note Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void
  copyReverse(const AudioArray &source, size_t sourceStart, size_t destinationStart, size_t length);

  /// @brief Copies data to a raw float pointer from this AudioArray.
  /// @param destination The destination float pointer to copy to.
  /// @param sourceStart The starting index in the this AudioArray.
  /// @param destinationStart The starting index in the destination float pointer.
  /// @param length The number of samples to copy.
  /// @note Assumes that destination and this are located in two distinct, non-overlapping memory locations.
  void copyTo(float *destination, size_t sourceStart, size_t destinationStart, size_t length) const;

  /// @brief Copies a sub-section of the array to another location within itself.
  /// @param sourceStart The index where the data to be copied begins.
  /// @param destinationStart The index where the data should be placed.
  /// @param length The number of samples to copy.
  void copyWithin(size_t sourceStart, size_t destinationStart, size_t length);

  void reverse();
  void normalize();
  void scale(float value);
  [[nodiscard]] float getMaxAbsValue() const;
  [[nodiscard]] float computeConvolution(const AudioArray &kernel, size_t startIndex = 0) const;

 protected:
  std::unique_ptr<float[]> data_ = nullptr;
  size_t size_ = 0;
};

} // namespace audioapi
