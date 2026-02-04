#pragma once

#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioArrayBuffer.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

namespace audioapi {

struct AudioBufferOptions;

class AudioBuffer {
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

  explicit AudioBuffer() = default;
  explicit AudioBuffer(size_t size, int numberOfChannels, float sampleRate);
  explicit AudioBuffer(const AudioBufferOptions &options);
  AudioBuffer(const AudioBuffer &other);
  AudioBuffer(AudioBuffer &&other) noexcept;
  AudioBuffer &operator=(const AudioBuffer &other);
  AudioBuffer &operator=(AudioBuffer &&other) noexcept;
  ~AudioBuffer() = default;

  [[nodiscard]] inline int getNumberOfChannels() const noexcept {
    return numberOfChannels_;
  }
  [[nodiscard]] inline float getSampleRate() const noexcept {
    return sampleRate_;
  }
  [[nodiscard]] inline size_t getSize() const noexcept {
    return size_;
  }

  [[nodiscard]] double getDuration() const noexcept {
    return static_cast<double>(size_) / getSampleRate();
  }

  /// @brief Get the AudioArray for a specific channel index.
  /// @param index The channel index.
  /// @return Pointer to the AudioArray for the specified channel - not owning.
  [[nodiscard]] AudioArray *getChannel(int index) const;

  /// @brief Get the AudioArray for a specific channel type.
  /// @param channelType The channel type (e.g., ChannelLeft, ChannelRight).
  /// @return Pointer to the AudioArray for the specified channel type - not owning.
  [[nodiscard]] AudioArray *getChannelByType(int channelType) const;

  /// @brief Get a copy of shared pointer to the AudioArray for a specific channel index.
  /// @param index The channel index.
  /// @return Copy of shared pointer to the AudioArray for the specified channel
  [[nodiscard]] std::shared_ptr<AudioArrayBuffer> getSharedChannel(int index) const;

  AudioArray &operator[](size_t index) {
    return *channels_[index];
  }
  const AudioArray &operator[](size_t index) const {
    return *channels_[index];
  }

  void zero();
  void zero(size_t start, size_t length);

  void sum(
      const AudioBuffer &source,
      ChannelInterpretation interpretation = ChannelInterpretation::SPEAKERS);
  void sum(
      const AudioBuffer &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length,
      ChannelInterpretation interpretation = ChannelInterpretation::SPEAKERS);

  void copy(const AudioBuffer &source);
  void copy(const AudioBuffer &source, size_t sourceStart, size_t destinationStart, size_t length);

  /// @brief Interleave audio data from this AudioBuffer into a destination buffer.
  /// @param destination Pointer to the destination buffer where interleaved audio data will be written.
  /// @param frames Number of frames to interleave into the destination buffer.
  /// @note The destination buffer should have enough space to hold the interleaved data
  /// according to the number of channels in this AudioBuffer.
  /// Example of interleaved data for stereo (2 channels):
  /// [L0, R0, L1, R1, L2, R2, ...]
  void interleaveTo(float *destination, size_t frames) const;

  void normalize();
  void scale(float value);
  [[nodiscard]] float maxAbsValue() const;

 private:
  std::vector<std::shared_ptr<AudioArrayBuffer>> channels_;

  int numberOfChannels_ = 0;
  float sampleRate_ = 0.0f;
  size_t size_ = 0;

  void createChannels();
  void discreteSum(
      const AudioBuffer &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length) const;
  void sumByUpMixing(
      const AudioBuffer &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length);
  void sumByDownMixing(
      const AudioBuffer &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length);
};

} // namespace audioapi
