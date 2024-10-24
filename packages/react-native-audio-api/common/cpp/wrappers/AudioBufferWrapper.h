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
  void copyFromChannel(
      float *destination,
      int destinationLength,
      int channelNumber,
      int startInChannel = 0) const;
  void copyToChannel(
      float *source,
      int sourceLength,
      int channelNumber,
      int startInChannel = 0) const;
};
} // namespace audioapi
