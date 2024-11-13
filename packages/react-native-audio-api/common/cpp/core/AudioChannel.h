#pragma once

#include <memory>
#include "AudioArray.h"

namespace audioapi {

class AudioChannel {
  public:
    explicit AudioChannel(int length, int sampleRate);

    [[nodiscard]] int getLength() const;
    [[nodiscard]] int getSampleRate() const;

    void zero();
    void copy(const AudioChannel &source);

    float& operator[](int index);
    const float& operator[](int index) const;


    float getMaxAbsValue() const;

    void normalize();
    void scale(float scale);
    void sum(const AudioChannel &source);


  private:

    int length_;
    int sampleRate_;

    std::unique_ptr<AudioArray> data_;
};

} // namespace audioapi
