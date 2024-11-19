#pragma once

#include <memory>
#include <algorithm>

namespace audioapi {

class AudioArray {
 public:
  explicit AudioArray(int size);
  ~AudioArray();

  [[nodiscard]] int getSize() const;
  float* getData() const;


  float& operator[](int index);
  const float& operator[](int index) const;

  void normalize();
  void resize(int size);
  void scale(float value);
  float getMaxAbsValue() const;

  void zero();
  void zero(int start, int length);

  void sum(const AudioArray* source);
  void sum(const AudioArray* source, int start, int length);
  void sum(const AudioArray* source, int sourceStart, int destinationStart, int length);

  void copy(const AudioArray* source);
  void copy(const AudioArray* source, int start, int length);
  void copy(const AudioArray* source, int sourceStart, int destinationStart, int length);


 private:
  float *data_;
  int size_;
};

} // namespace audioapi
