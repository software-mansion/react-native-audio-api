#pragma once

#include <audioapi/utils/AudioArray.hpp>

#if !RN_AUDIO_API_TEST
#include <jsi/jsi.h>
using JsiBuffer = facebook::jsi::MutableBuffer;
#else
// Dummy class to inherit from nothing if testing
struct JsiBuffer {
  virtual size_t size() const = 0;
  virtual uint8_t *data() = 0;
};
#endif

namespace audioapi {

template <size_t Alignment>
class AlignedAudioArrayBuffer : public JsiBuffer, public AlignedAudioArray<Alignment> {
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
