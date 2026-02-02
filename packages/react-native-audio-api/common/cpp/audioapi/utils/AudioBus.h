#pragma once

#include <audioapi/core/types/ChannelInterpretation.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

namespace audioapi {

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

  explicit AudioBus() = default;
  explicit AudioBus(size_t size, int numberOfChannels, float sampleRate);
  AudioBus(const AudioBus &other);
  AudioBus(AudioBus &&other) noexcept;
  AudioBus &operator=(const AudioBus &other);
  AudioBus &operator=(AudioBus &&other) noexcept;
  ~AudioBus() = default;

  [[nodiscard]] inline int getNumberOfChannels() const noexcept {
    return numberOfChannels_;
  }
  [[nodiscard]] inline float getSampleRate() const noexcept {
    return sampleRate_;
  }
  [[nodiscard]] inline size_t getSize() const noexcept {
    return size_;
  }

  /// @brief Get the AudioArray for a specific channel index.
  /// @param index The channel index.
  /// @return Pointer to the AudioArray for the specified channel - not owning.
  [[nodiscard]] AudioArray *getChannel(int index) const;

  /// @brief Get the AudioArray for a specific channel type.
  /// @param channelType The channel type (e.g., ChannelLeft, ChannelRight).
  /// @return Pointer to the AudioArray for the specified channel type - not owning.
  [[nodiscard]] AudioArray *getChannelByType(int channelType) const;

  /// @brief Get a shared pointer to the AudioArray for a specific channel index.
  /// @param index The channel index.
  /// @return Shared pointer to the AudioArray for the specified channel - owning.
  [[nodiscard]] std::shared_ptr<AudioArray> getSharedChannel(int index) const;

  AudioArray &operator[](size_t index) {
    return *channels_[index];
  }
  const AudioArray &operator[](size_t index) const {
    return *channels_[index];
  }

  void zero();
  void zero(size_t start, size_t length);

  void sum(
      const AudioBus &source,
      ChannelInterpretation interpretation = ChannelInterpretation::SPEAKERS);
  void sum(
      const AudioBus &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length,
      ChannelInterpretation interpretation = ChannelInterpretation::SPEAKERS);

  void copy(const AudioBus &source);
  void copy(const AudioBus &source, size_t sourceStart, size_t destinationStart, size_t length);

  void normalize();
  void scale(float value);
  [[nodiscard]] float maxAbsValue() const;

 private:
  std::vector<std::shared_ptr<AudioArray>> channels_;

  int numberOfChannels_ = 0;
  float sampleRate_ = 0.0f;
  size_t size_ = 0;

  void createChannels();
  void discreteSum(
      const AudioBus &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length) const;
  void sumByUpMixing(
      const AudioBus &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length);
  void sumByDownMixing(
      const AudioBus &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length);
};

} // namespace audioapi
