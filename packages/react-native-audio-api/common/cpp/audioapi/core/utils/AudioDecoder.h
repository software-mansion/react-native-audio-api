#pragma once

#include <memory>
#include <string>

namespace audioapi {

class AudioBus;

class AudioDecoder {
 public:
  explicit AudioDecoder(float sampleRate): sampleRate_(sampleRate) {}

  [[nodiscard]] std::shared_ptr<AudioBus> decodeWithFilePath(const std::string &path) const;
  [[nodiscard]] std::shared_ptr<AudioBus> decodeWithMemoryBlock(const void *data, size_t size) const;

 private:
  float sampleRate_;
};

} // namespace audioapi
