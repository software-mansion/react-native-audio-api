#pragma once

#include <memory>
#include <vector>
#include <algorithm>

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
  AudioArray* getChannel(int index) const;
  AudioArray* getChannelByType(int channelType) const;

  void zero();
  void normalize();
  void scale(float value);
  float maxAbsValue() const;

  void copy(const AudioBus &source);
  void sum(const AudioBus &source);

 private:
  std::vector<std::unique_ptr<AudioArray>> channels_;

  int numberOfChannels_;
  int sampleRate_;
  int size_;

  void createChannels();
  void discreteSum(const AudioBus &source);
  void sumByUpMixing(const AudioBus &source);
  void sumByDownMixing(const AudioBus &source);
};

} // namespace audioapi
