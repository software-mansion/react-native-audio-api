#pragma once

#include <memory>

#include "AudioBuffer.h"

namespace audioapi {

class AudioBufferWrapper {
 public:
  explicit AudioBufferWrapper(const std::shared_ptr<AudioBuffer> &audioBuffer);

  std::shared_ptr<AudioBuffer> audioBuffer_;
  [[nodiscard]] int getNumberOfChannels() const;
  [[nodiscard]] int getLength() const;
  [[nodiscard]] double getDuration() const;
  [[nodiscard]] int getSampleRate() const;
  [[nodiscard]] float *getChannelData(int channel) const;
  void setChannelData(int channel, float *data, int length) const;
};
} // namespace audioapi
