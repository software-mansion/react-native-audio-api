#pragma once

#include <algorithm>
#include <memory>
#include <cstddef>
#include <cstring>

namespace audioapi {

class AudioArray {
 public:
  explicit AudioArray(size_t size);
  AudioArray(const AudioArray &other);
  ~AudioArray();

  [[nodiscard]] size_t getSize() const;
  [[nodiscard]] float *getData() const;

  float &operator[](size_t index);
  const float &operator[](size_t index) const;

  void normalize();
  void resize(size_t size);
  void scale(float value);
  [[nodiscard]] float getMaxAbsValue() const;

  void zero();
  void zero(size_t start, size_t length);

  void sum(const AudioArray *source);
  void sum(const AudioArray *source, size_t start, size_t length);
  void sum(
      const AudioArray *source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length);

  void copy(const AudioArray *source);
  void copy(const AudioArray *source, size_t start, size_t length);
  void copy(
      const AudioArray *source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length);

 protected:
  float *data_;
  size_t size_;
};

} // namespace audioapi
