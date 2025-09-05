#pragma once

#include <jsi/jsi.h>

namespace audioapi {

using namespace facebook;

class AudioArrayBuffer : public jsi::MutableBuffer {
 public:
  AudioArrayBuffer(uint8_t *data, size_t size): data_(data), size_(size) {}
  ~AudioArrayBuffer() override {
    if (data_ == nullptr) {
      return;
    }
    delete[] data_;
  }
  AudioArrayBuffer(AudioArrayBuffer &&other) noexcept
      : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;
  }

  AudioArrayBuffer(const AudioArrayBuffer &) = delete;
  AudioArrayBuffer &operator=(const AudioArrayBuffer &) = delete;
  AudioArrayBuffer &operator=(AudioArrayBuffer &&other) = delete;

  [[nodiscard]] size_t size() const override;
  uint8_t *data() override;

 private:
  uint8_t *data_;
  const size_t size_;
};

} // namespace audioapi

