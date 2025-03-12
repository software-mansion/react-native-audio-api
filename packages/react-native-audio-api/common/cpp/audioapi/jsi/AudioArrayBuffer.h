#pragma once

#include <jsi/jsi.h>

namespace audioapi {

using namespace facebook;

class AudioArrayBuffer : public jsi::MutableBuffer {
 public:
  AudioArrayBuffer(float *data, size_t size);
  ~AudioArrayBuffer() override = default;

  [[nodiscard]] size_t size() const override;
  uint8_t *data() override;

 private:
  float *data_;
  size_t size_;
};

} // namespace audioapi

