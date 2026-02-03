#pragma once

#include <audioapi/utils/AudioArray.h>
#include <memory>
#include <utility>

#if !RN_AUDIO_API_TEST
#include <jsi/jsi.h>

namespace audioapi {

using namespace facebook;

class AudioArrayBuffer : public jsi::MutableBuffer, public AudioArray {
 public:
  explicit AudioArrayBuffer(size_t size) : AudioArray(size) {};
  AudioArrayBuffer(const float *data, size_t size) : AudioArray(data, size) {};

  [[nodiscard]] size_t size() const override {
    return size_ * sizeof(float);
  }
  uint8_t *data() override {
    return reinterpret_cast<uint8_t *>(data_.get());
  }
};

} // namespace audioapi

#else

namespace audioapi {

class AudioArrayBuffer : public AudioArray {
 public:
  explicit AudioArrayBuffer(size_t size) : AudioArray(size) {};
  AudioArrayBuffer(const float *data, size_t size) : AudioArray(data, size) {};

  [[nodiscard]] size_t size() const {
    return size_ * sizeof(float);
  }
  uint8_t *data() {
    return reinterpret_cast<uint8_t *>(data_.get());
  }
};

} // namespace audioapi

#endif
