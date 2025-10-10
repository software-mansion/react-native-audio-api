#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstring>

namespace audioapi {

class AudioBus;

class AudioBuffer : public std::enable_shared_from_this<AudioBuffer> {
 public:
  explicit AudioBuffer(int numberOfChannels, size_t length, float sampleRate);
  explicit AudioBuffer(std::shared_ptr<AudioBus> bus);

  [[nodiscard]] size_t getLength() const;
  [[nodiscard]] float getSampleRate() const;
  [[nodiscard]] double getDuration() const;

  [[nodiscard]] int getNumberOfChannels() const;
  [[nodiscard]] float *getChannelData(int channel) const;

  void copyFromChannel(
      float *destination,
      size_t destinationLength,
      int channelNumber,
      size_t startInChannel) const;
  void copyToChannel(
      const float *source,
      size_t sourceLength,
      int channelNumber,
      size_t startInChannel);

 private:
  friend class AudioBufferSourceNode;
  friend class AudioBufferQueueSourceNode;
  friend class AudioBufferHostObject;

  std::shared_ptr<AudioBus> bus_;
};

} // namespace audioapi
