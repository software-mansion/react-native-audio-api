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
    AudioBuffer(AudioBus *bus);

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
  std::shared_ptr<AudioBus> bus_;

  friend class AudioBufferSourceNode;
};

} // namespace audioapi
