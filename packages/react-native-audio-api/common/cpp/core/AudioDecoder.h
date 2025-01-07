#pragma once

#include <string>

namespace audioapi {

class AudioBus;

class AudioDecoder {
 public:
  explicit AudioDecoder(int sampleRate) : sampleRate_(sampleRate) {}

  [[nodiscard]] AudioBus *decodeWithFilePath(const std::string &path) const;

 private:
  int sampleRate_;
};

} // namespace audioapi
