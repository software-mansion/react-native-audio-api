#pragma once

#include <memory>

namespace audioapi {

class AudioArray {
  public:
    explicit AudioArray(int size);
    ~AudioArray();

    [[nodiscard]] int getSize() const;


    float& operator[](int index);
    const float& operator[](int index) const;

    void zero();
    void resize(int size);
    void copy(const AudioArray &source);

  private:
    float *data_;
    int size_;
};

}
