#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class AudioBus;

class AudioBuffer : public std::enable_shared_from_this<AudioBuffer> {
 public:
  explicit AudioBuffer(int numberOfChannels, int length, int sampleRate);

  [[nodiscard]] int getLength() const;
  [[nodiscard]] int getSampleRate() const;
  [[nodiscard]] double getDuration() const;

  [[nodiscard]] int getNumberOfChannels() const;
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
  std::unique_ptr<AudioBus> bus_;
};

} // namespace audioapi
