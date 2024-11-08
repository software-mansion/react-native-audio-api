#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class AudioBuffer : public std::enable_shared_from_this<AudioBuffer> {
 public:
  explicit AudioBuffer(int numberOfChannels, int length, int sampleRate);

  [[nodiscard]] int getNumberOfChannels() const;
  [[nodiscard]] int getLength() const;
  [[nodiscard]] int getSampleRate() const;
  [[nodiscard]] double getDuration() const;
  [[nodiscard]] float *getChannelData(int channel) const;

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

  int numberOfChannels_;
  int length_;
  int sampleRate_;
  double duration_;
  float **channels_;

  std::shared_ptr<AudioBuffer> mix(int outputNumberOfChannels);
};

} // namespace audioapi
