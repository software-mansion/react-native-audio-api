#pragma once

#include <audioapi/utils/AudioArray.hpp>

#include <jsi/jsi.h>

namespace audioapi {

template <size_t Alignment>
class AlignedAudioArrayBuffer : public facebook::jsi::MutableBuffer,
                                public AlignedAudioArray<Alignment> {
 public:
  explicit AlignedAudioArrayBuffer(size_t size) : AlignedAudioArray<Alignment>(size) {};
  AlignedAudioArrayBuffer(const float *data, size_t size)
      : AlignedAudioArray<Alignment>(data, size) {};

  [[nodiscard]] size_t size() const override {
    return this->size_ * sizeof(float);
  }
  uint8_t *data() override {
    return reinterpret_cast<uint8_t *>(this->data_.data());
  }
};

using AudioArrayBuffer = AlignedAudioArrayBuffer<alignof(std::max_align_t)>;

} // namespace audioapi
