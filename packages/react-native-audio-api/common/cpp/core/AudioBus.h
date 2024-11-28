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

  explicit AudioBus(int sampleRate, int size);
  explicit AudioBus(int sampleRate, int size, int numberOfChannels);

  ~AudioBus();

  [[nodiscard]] int getNumberOfChannels() const;
  [[nodiscard]] int getSampleRate() const;
  [[nodiscard]] int getSize() const;
  AudioArray *getChannel(int index) const;
  AudioArray *getChannelByType(int channelType) const;

  void normalize();
  void scale(float value);
  float maxAbsValue() const;

  void zero();
  void zero(int start, int length);

  void sum(const AudioBus *source);
  void sum(const AudioBus *source, int start, int length);
  void sum(
      const AudioBus *source,
      int sourceStart,
      int destinationStart,
      int length);

  void copy(const AudioBus *source);
  void copy(const AudioBus *source, int start, int length);
  void copy(
      const AudioBus *source,
      int sourceStart,
      int destinationStart,
      int length);

 private:
  std::vector<std::shared_ptr<AudioArray>> channels_;

  int numberOfChannels_;
  int sampleRate_;
  int size_;

  void createChannels();
  void discreteSum(
      const AudioBus *source,
      int sourceStart,
      int destinationStart,
      int length) const;
  void sumByUpMixing(
      const AudioBus *source,
      int sourceStart,
      int destinationStart,
      int length);
  void sumByDownMixing(
      const AudioBus *source,
      int sourceStart,
      int destinationStart,
      int length);
};

} // namespace audioapi
