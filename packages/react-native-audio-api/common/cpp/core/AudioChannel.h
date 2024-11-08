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
    void scale(float scale);

    void sumFrom(const AudioChannel &source);
    void copyFrom(const AudioChannel &source);

    float maxAbsValue() const;

  private:

    int length_;
    int sampleRate_;

    std::unique_ptr<AudioArray> data_;
};

} // namespace audioapi
