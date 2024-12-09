#pragma once

#include <string>

namespace audioapi {

class AudioBus;

class AudioDecoder {
 public:
  explicit AudioDecoder(int sampleRate);

  [[nodiscard]] AudioBus *decodeWithFilePath(const std::string &path) const;
  // TODO: implement this
  [[nodiscard]] AudioBus *decodeWithArrayBuffer() const;

 private:
  int sampleRate_;
};

} // namespace audioapi
