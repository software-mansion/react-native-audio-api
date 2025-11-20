#pragma once

#include <audioapi/utils/AudioArray.h>
#include <jsi/jsi.h>

#include <memory>
#include <utility>

namespace audioapi {

using namespace facebook;

class AudioArrayBuffer : public jsi::MutableBuffer {
 public:
  explicit AudioArrayBuffer(const std::shared_ptr<AudioArray> &audioArray)
      : audioArray_(audioArray) {}
  ~AudioArrayBuffer() override = default;

  AudioArrayBuffer(AudioArrayBuffer &&other) noexcept : audioArray_(std::move(other.audioArray_)) {
    other.audioArray_ = nullptr;
  }

  AudioArrayBuffer(const AudioArrayBuffer &) = delete;
  AudioArrayBuffer &operator=(const AudioArrayBuffer &) = delete;
  AudioArrayBuffer &operator=(AudioArrayBuffer &&other) = delete;

  [[nodiscard]] size_t size() const override;
  uint8_t *data() override;

 private:
  std::shared_ptr<AudioArray> audioArray_;
};

} // namespace audioapi
