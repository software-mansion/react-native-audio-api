#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class AudioBuffer : public std::enable_shared_from_this<AudioBuffer> {
 public:
  explicit AudioBuffer(int numberOfChannels, int length, int sampleRate);

  int getNumberOfChannels() const;
  int getLength() const;
  int getSampleRate() const;
  double getDuration() const;
  float *getChannelData(int channel) const;
  void setChannelData(int channel, const float *data, int length);
  void copyFromChannel(
      float *destination,
      int destinationLength,
      int channelNumber,
      int startInChannel) const;
  void copyToChannel(
      const float *source,
      int sourceLength,
      int channelNumber,
      int startInChannel);

 private:
  friend class AudioBufferSourceNode;

  int numberOfChannels_;
  int length_;
  int sampleRate_;
  double duration_;
  float **channels_;

  std::shared_ptr<AudioBuffer> mix(int outputNumberOfChannels);
};

} // namespace audioapi
