#pragma once

#include <memory>
#include <string>
#include <vector>

namespace audioapi {

// TODO implement AudioBuffer class

class AudioBuffer {
 public:
  explicit AudioBuffer(int numberOfChannels, int length, int sampleRate);

  int getNumberOfChannels() const;
  int getLength() const;
  int getSampleRate() const;
  double getDuration() const;
  float *getChannelData(int channel) const;
  void setChannelData(int channel, const float *data, int length);

 private:
  int numberOfChannels_;
  int length_;
  int sampleRate_;
  double duration_;
  std::vector<std::vector<float>> channels_;
};

} // namespace audioapi
