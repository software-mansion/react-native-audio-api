#pragma once

#include <algorithm>
#include <memory>
#include <vector>

namespace audioapi {

class BaseAudioContext;
class AudioArray;

class AudioBus {
 public:
  enum {
    ChannelMono = 0,
    ChannelLeft = 0,
    ChannelRight = 1,
    ChannelCenter = 2,
    ChannelLFE = 3,
    ChannelSurroundLeft = 4,
    ChannelSurroundRight = 5,
  };

  explicit AudioBus(float sampleRate, size_t size, int numberOfChannels);

  ~AudioBus();

  [[nodiscard]] int getNumberOfChannels() const;
  [[nodiscard]] float getSampleRate() const;
  [[nodiscard]] size_t getSize() const;
  [[nodiscard]] AudioArray *getChannel(int index) const;
  [[nodiscard]] AudioArray *getChannelByType(int channelType) const;

  void normalize();
  void scale(float value);
  [[nodiscard]] float maxAbsValue() const;

  void zero();
  void zero(size_t start, size_t length);

  void sum(const AudioBus *source);
  void sum(const AudioBus *source, size_t start, size_t length);
  void sum(
      const AudioBus *source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length);

  void copy(const AudioBus *source);
  void copy(const AudioBus *source, size_t start, size_t length);
  void copy(
      const AudioBus *source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length);

 private:
  std::vector<std::shared_ptr<AudioArray>> channels_;

  int numberOfChannels_;
  float sampleRate_;
  size_t size_;

  void createChannels();
  void discreteSum(
      const AudioBus *source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length) const;
  void sumByUpMixing(
      const AudioBus *source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length);
  void sumByDownMixing(
      const AudioBus *source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length);
};

} // namespace audioapi
