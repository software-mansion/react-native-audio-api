#pragma once

#include <audioapi/utils/AudioArray.hpp>
#include <concepts>

namespace audioapi {

template <typename T>
concept AudioArrayBase = requires(T a, const float *c_data, float *data, size_t size) {
  { a.begin() } -> std::same_as<float *>;
  { a.getSize() } -> std::same_as<size_t>;
  a.copy(c_data, size, size, size);
  a.copyTo(data, size, size, size);
} && std::constructible_from<T, size_t>;

template <AudioArrayBase Base>
class CircularArray : public Base {
 public:
  explicit CircularArray(size_t size) : Base(size) {}

  CircularArray(const CircularArray &other) = default;
  CircularArray(CircularArray &&other) noexcept = default;
  CircularArray &operator=(const CircularArray &other) = default;
  CircularArray &operator=(CircularArray &&other) noexcept = default;
  ~CircularArray() = default;

  void push_back(const Base &data, size_t size, bool skipAvailableSpaceCheck = false) {
    push_back(data.begin(), size, skipAvailableSpaceCheck);
  }

  void push_back(const float *data, size_t size, bool skipAvailableSpaceCheck = false) {
    if (size > this->size_) {
      throw std::overflow_error("size exceeds CircularArray size_");
    }

    if (size > getAvailableSpace() && !skipAvailableSpaceCheck) {
      throw std::overflow_error("not enough space in CircularArray");
    }

    if (vWriteIndex_ + size > this->size_) {
      auto partSize = this->size_ - vWriteIndex_;
      this->copy(data, 0, vWriteIndex_, partSize);    // NOLINT(build/include_what_you_use)
      this->copy(data, partSize, 0, size - partSize); // NOLINT(build/include_what_you_use)
    } else {
      this->copy(data, 0, vWriteIndex_, size); // NOLINT(build/include_what_you_use)
    }

    vWriteIndex_ =
        vWriteIndex_ + size > this->size_ ? vWriteIndex_ + size - this->size_ : vWriteIndex_ + size;
  }

  void pop_front(Base &data, size_t size, bool skipAvailableDataCheck = false) {
    pop_front(data.begin(), size, skipAvailableDataCheck);
  }

  void pop_front(float *data, size_t size, bool skipAvailableDataCheck = false) {
    if (size > this->size_) {
      throw std::overflow_error("size exceeds CircularArray size_");
    }

    if (size > getNumberOfAvailableFrames() && !skipAvailableDataCheck) {
      throw std::overflow_error("not enough data in CircularArray");
    }

    if (vReadIndex_ + size > this->size_) {
      auto partSize = this->size_ - vReadIndex_;
      this->copyTo(data, vReadIndex_, 0, partSize);
      this->copyTo(data, 0, partSize, size - partSize);
    } else {
      this->copyTo(data, vReadIndex_, 0, size);
    }

    vReadIndex_ =
        vReadIndex_ + size > this->size_ ? vReadIndex_ + size - this->size_ : vReadIndex_ + size;
  }

  void pop_back(Base &data, size_t size, size_t offset = 0, bool skipAvailableDataCheck = false) {
    pop_back(data.begin(), size, offset, skipAvailableDataCheck);
  }

  void pop_back(float *data, size_t size, size_t offset = 0, bool skipAvailableDataCheck = false) {
    if (size > this->size_) {
      throw std::overflow_error("size exceeds CircularArray size_");
    }

    if (size + offset > getNumberOfAvailableFrames() && !skipAvailableDataCheck) {
      throw std::overflow_error("not enough data in CircularArray");
    }

    if (vWriteIndex_ <= offset) {
      this->copyTo(data, this->size_ - (offset - vWriteIndex_) - size, 0, size);
    } else if (vWriteIndex_ <= size + offset) {
      auto partSize = size + offset - vWriteIndex_;
      this->copyTo(data, this->size_ - partSize, 0, partSize);
      this->copyTo(data, 0, partSize, size - partSize);
    } else {
      this->copyTo(data, vWriteIndex_ - size - offset, 0, size);
    }

    vReadIndex_ = vWriteIndex_ < offset ? size + vWriteIndex_ - offset : vWriteIndex_ - offset;
  }

  [[nodiscard]] size_t getNumberOfAvailableFrames() const {
    return vWriteIndex_ >= vReadIndex_ ? vWriteIndex_ - vReadIndex_
                                       : this->size_ - vReadIndex_ + vWriteIndex_;
  }

 private:
  size_t vWriteIndex_ = 0;
  size_t vReadIndex_ = 0;

  [[nodiscard]] size_t getAvailableSpace() const {
    return this->size_ - getNumberOfAvailableFrames();
  }
};

using CircularAudioArray = CircularArray<AudioArray>;
using CircularDSPAudioArray = CircularArray<DSPAudioArray>;

} // namespace audioapi
