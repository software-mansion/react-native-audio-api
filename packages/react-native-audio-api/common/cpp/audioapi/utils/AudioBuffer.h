#pragma once

#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioArrayBuffer.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace audioapi {

class AudioArrayBuffer;

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
  AudioBuffer(const AudioBuffer &other);
  AudioBuffer(AudioBuffer &&other) noexcept;
  AudioBuffer &operator=(const AudioBuffer &other);
  AudioBuffer &operator=(AudioBuffer &&other) noexcept;
  ~AudioBuffer() = default;

  [[nodiscard]] size_t getNumberOfChannels() const noexcept {
    return numberOfChannels_;
  }
  [[nodiscard]] float getSampleRate() const noexcept {
    return sampleRate_;
  }
  [[nodiscard]] size_t getSize() const noexcept {
    return size_;
  }

  [[nodiscard]] double getDuration() const noexcept {
    return static_cast<double>(size_) / static_cast<double>(getSampleRate());
  }

  /// @brief Get the AudioArray for a specific channel index.
  /// @param index The channel index.
  /// @return Pointer to the AudioArray for the specified channel - not owning.
  [[nodiscard]] AudioArray *getChannel(size_t index) const;

  /// @brief Get the AudioArray for a specific channel type.
  /// @param channelType The channel type: ChannelMono = 0, ChannelLeft = 0,
  /// ChannelRight = 1, ChannelCenter = 2, ChannelLFE = 3,
  /// ChannelSurroundLeft = 4, ChannelSurroundRight = 5
  /// @return Pointer to the AudioArray for the specified channel type - not owning.
  [[nodiscard]] AudioArray *getChannelByType(int channelType) const;

  /// @brief Get a copy of shared pointer to the AudioArray for a specific channel index.
  /// @param index The channel index.
  /// @return Copy of shared pointer to the AudioArray for the specified channel
  [[nodiscard]] std::shared_ptr<AudioArrayBuffer> getSharedChannel(size_t index) const;

  AudioArray &operator[](size_t index) {
    return *channels_[index];
  }
  const AudioArray &operator[](size_t index) const {
    return *channels_[index];
  }

  void zero();
  void zero(size_t start, size_t length);

  /// @brief Sums audio data from a source AudioBuffer into this AudioBuffer.
  /// @param source The source AudioBuffer to sum from.
  /// @param interpretation The channel interpretation to use for summing (default is SPEAKERS).
  /// @note Handles up-mixing and down-mixing based on the number of channels in both buffers.
  /// Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void sum(
      const AudioBuffer &source,
      ChannelInterpretation interpretation = ChannelInterpretation::SPEAKERS);

  /// @brief Sums audio data from a source AudioBuffer into this AudioBuffer.
  /// @param source The source AudioBuffer to sum from.
  /// @param sourceStart The starting index in the source AudioBuffer.
  /// @param destinationStart The starting index in this AudioBuffer.
  /// @param length The number of samples to sum.
  /// @param interpretation The channel interpretation to use for summing (default is SPEAKERS).
  /// @note Handles up-mixing and down-mixing based on the number of channels in both buffers.
  /// Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void sum(
      const AudioBuffer &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length,
      ChannelInterpretation interpretation = ChannelInterpretation::SPEAKERS);

  /// @brief Copies audio data from a source AudioBuffer into this AudioBuffer.
  /// @param source The source AudioBuffer to copy from.
  /// @note Handles up-mixing and down-mixing based on the number of channels in both buffers.
  /// Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void copy(const AudioBuffer &source);

  /// @brief Copies audio data from a source AudioBuffer into this AudioBuffer.
  /// @param source The source AudioBuffer to copy from.
  /// @param sourceStart The starting index in the source AudioBuffer.
  /// @param destinationStart The starting index in this AudioBuffer.
  /// @param length The number of samples to copy.
  /// @note Handles up-mixing and down-mixing based on the number of channels in both buffers.
  /// Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void copy(const AudioBuffer &source, size_t sourceStart, size_t destinationStart, size_t length);

  /// @brief Deinterleave audio data from a source buffer into this AudioBuffer.
  /// @param source Pointer to the source buffer containing interleaved audio data.
  /// @param frames Number of frames to deinterleave from the source buffer.
  /// @note The source buffer should contain interleaved audio data according to the number of channels in this AudioBuffer.
  /// Example of interleaved data for stereo (2 channels):
  /// [L0, R0, L1, R1, L2, R2, ...]
  /// Assumes that source and this are located in two distinct, non-overlapping memory locations.
  void deinterleaveFrom(const float *source, size_t frames);

  /// @brief Interleave audio data from this AudioBuffer into a destination buffer.
  /// @param destination Pointer to the destination buffer where interleaved audio data will be written.
  /// @param frames Number of frames to interleave into the destination buffer.
  /// @note The destination buffer should have enough space to hold the interleaved data
  /// according to the number of channels in this AudioBuffer.
  /// Example of interleaved data for stereo (2 channels):
  /// [L0, R0, L1, R1, L2, R2, ...]
  /// Assumes that this and destination are located in two distinct, non-overlapping memory locations.
  void interleaveTo(float *destination, size_t frames) const;

  void normalize();
  void scale(float value);
  [[nodiscard]] float maxAbsValue() const;

 private:
  std::vector<std::shared_ptr<AudioArrayBuffer>> channels_;

  size_t numberOfChannels_ = 0;
  float sampleRate_ = 0.0f;
  size_t size_ = 0;

  inline static const std::unordered_map<size_t, std::vector<int>> kChannelLayouts = {
      {1, {ChannelMono}},
      {2, {ChannelLeft, ChannelRight}},
      {4, {ChannelLeft, ChannelRight, ChannelSurroundLeft, ChannelSurroundRight}},
      {5, {ChannelLeft, ChannelRight, ChannelCenter, ChannelSurroundLeft, ChannelSurroundRight}},
      {6,
       {ChannelLeft,
        ChannelRight,
        ChannelCenter,
        ChannelLFE,
        ChannelSurroundLeft,
        ChannelSurroundRight}}};

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
