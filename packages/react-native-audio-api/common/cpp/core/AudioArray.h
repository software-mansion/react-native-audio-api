#pragma once

#include <memory>

namespace audioapi {

class AudioArray {
 public:
  explicit AudioArray(int size);
  ~AudioArray();

  [[nodiscard]] int getSize() const;
  float* getData() const;


  float& operator[](int index);
  const float& operator[](int index) const;

  void zero();
  void resize(int size);
  void copy(const AudioArray* source);

  float getMaxAbsValue() const;

  void normalize();
  void scale(float value);
  void sum(const AudioArray* source);


 private:
  float *data_;
  int size_;
};

} // namespace audioapi
