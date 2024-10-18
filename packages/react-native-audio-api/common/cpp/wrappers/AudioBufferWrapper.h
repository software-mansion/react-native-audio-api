#pragma once

#include <memory>

#include "AudioBuffer.h"

namespace audioapi {

class AudioBufferWrapper {
 public:
  explicit AudioBufferWrapper(const std::shared_ptr<AudioBuffer> &audioBuffer);

  std::shared_ptr<AudioBuffer> audioBuffer_;
  int getNumberOfChannels() const;
  int getLength() const;
  double getDuration() const;
  int getSampleRate() const;
  float *getChannelData(int channel) const;
  void setChannelData(int channel, float *data, int length) const;
};
} // namespace audioapi
