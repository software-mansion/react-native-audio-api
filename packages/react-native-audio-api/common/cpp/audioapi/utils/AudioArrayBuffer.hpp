#pragma once

#include <audioapi/utils/AudioArray.h>
#include <memory>
#include <utility>

#if !RN_AUDIO_API_TEST
#include <jsi/jsi.h>
using JsiBuffer = facebook::jsi::MutableBuffer;
#else
// Dummy class to inherit from nothing if testing
struct JsiBuffer {};
#endif

namespace audioapi {

class AudioArrayBuffer : public JsiBuffer, public AudioArray {
 public:
  explicit AudioArrayBuffer(size_t size) : AudioArray(size) {};
  AudioArrayBuffer(const float *data, size_t size) : AudioArray(data, size) {};

#if !RN_AUDIO_API_TEST
  [[nodiscard]] size_t size() const override {
    return size_ * sizeof(float);
  }
  uint8_t *data() override {
    return reinterpret_cast<uint8_t *>(data_.get());
  }
#else
  [[nodiscard]] size_t size() const {
    return size_ * sizeof(float);
  }
  uint8_t *data() {
    return reinterpret_cast<uint8_t *>(data_.get());
  }
#endif
};

} // namespace audioapi
