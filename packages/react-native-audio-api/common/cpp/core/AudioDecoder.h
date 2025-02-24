#pragma once

#include <string>

namespace audioapi {

class AudioBus;

class AudioDecoder {
 public:
  explicit AudioDecoder(float sampleRate) : sampleRate_(sampleRate) {}

  [[nodiscard]] std::shared_ptr<AudioBus> decodeWithFilePath(const std::string &path) const;

 private:
  float sampleRate_;
};

} // namespace audioapi
