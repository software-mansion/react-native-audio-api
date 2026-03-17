#pragma once

#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioArrayBuffer.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace audioapi {

/// @brief AlignedAudioBuffer is a multi-channel audio buffer backed by AlignedAudioArray channels.
/// @tparam Alignment The memory alignment in bytes for the underlying channel arrays.
template <size_t Alignment>
class AlignedAudioBuffer {
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

  template <size_t A>
  friend class AlignedAudioBuffer;

  explicit AlignedAudioBuffer() = default;

  explicit AlignedAudioBuffer(size_t size, int numberOfChannels, float sampleRate)
      : numberOfChannels_(numberOfChannels), sampleRate_(sampleRate), size_(size) {
    channels_.reserve(numberOfChannels_);
    for (size_t i = 0; i < numberOfChannels_; ++i) {
      channels_.emplace_back(std::make_shared<AlignedAudioArrayBuffer<Alignment>>(size_));
    }
  }

  AlignedAudioBuffer(const AlignedAudioBuffer &other)
      : numberOfChannels_(other.numberOfChannels_),
        sampleRate_(other.sampleRate_),
        size_(other.size_) {
    channels_.reserve(numberOfChannels_);
    for (const auto &channel : other.channels_) {
      channels_.emplace_back(std::make_shared<AlignedAudioArrayBuffer<Alignment>>(*channel));
    }
  }

  AlignedAudioBuffer(AlignedAudioBuffer &&other) noexcept
      : channels_(std::move(other.channels_)),
        numberOfChannels_(std::exchange(other.numberOfChannels_, 0)),
        sampleRate_(std::exchange(other.sampleRate_, 0.0f)),
        size_(std::exchange(other.size_, 0)) {}

  AlignedAudioBuffer &operator=(const AlignedAudioBuffer &other) {
    if (this != &other) {
      sampleRate_ = other.sampleRate_;

      if (numberOfChannels_ != other.numberOfChannels_) {
        numberOfChannels_ = other.numberOfChannels_;
        size_ = other.size_;
        channels_.clear();
        channels_.reserve(numberOfChannels_);

        for (const auto &channel : other.channels_) {
          channels_.emplace_back(std::make_shared<AlignedAudioArrayBuffer<Alignment>>(*channel));
        }

        return *this;
      }

      if (size_ != other.size_) {
        size_ = other.size_;
      }

      for (size_t i = 0; i < numberOfChannels_; ++i) {
        *channels_[i] = *other.channels_[i];
      }
    }

    return *this;
  }

  AlignedAudioBuffer &operator=(AlignedAudioBuffer &&other) noexcept {
    if (this != &other) {
      channels_ = std::move(other.channels_);
      numberOfChannels_ = std::exchange(other.numberOfChannels_, 0);
      sampleRate_ = std::exchange(other.sampleRate_, 0.0f);
      size_ = std::exchange(other.size_, 0);
    }
    return *this;
  }

  ~AlignedAudioBuffer() = default;

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

  /// @brief Get the channel array for a specific channel index.
  /// @return Pointer to the AlignedAudioArray for the specified channel - not owning.
  [[nodiscard]] AlignedAudioArray<Alignment> *getChannel(size_t index) const {
    return channels_[index].get();
  }

  /// @brief Get the channel array for a specific channel type.
  /// @return Pointer to the AlignedAudioArray for the specified channel type - not owning.
  [[nodiscard]] AlignedAudioArray<Alignment> *getChannelByType(int channelType) const {
    auto it = kChannelLayouts.find(getNumberOfChannels());
    if (it == kChannelLayouts.end()) {
      return nullptr;
    }
    const auto &channelOrder = it->second;
    for (size_t i = 0; i < channelOrder.size(); ++i) {
      if (channelOrder[i] == channelType) {
        return getChannel(i);
      }
    }
    return nullptr;
  }

  /// @brief Get a copy of shared pointer to the channel buffer for a specific index.
  [[nodiscard]] std::shared_ptr<AlignedAudioArrayBuffer<Alignment>> getSharedChannel(
      size_t index) const {
    return channels_[index];
  }

  AlignedAudioArray<Alignment> &operator[](size_t index) {
    return *channels_[index];
  }
  const AlignedAudioArray<Alignment> &operator[](size_t index) const {
    return *channels_[index];
  }

  void zero() {
    zero(0, getSize());
  }

  void zero(size_t start, size_t length) {
    for (auto it = channels_.begin(); it != channels_.end(); it += 1) {
      it->get()->zero(start, length);
    }
  }

  /// @brief Sums audio data from a source buffer into this buffer.
  /// @note Handles up-mixing and down-mixing based on channel counts.
  template <size_t OtherAlignment>
  void sum(
      const AlignedAudioBuffer<OtherAlignment> &source,
      ChannelInterpretation interpretation = ChannelInterpretation::SPEAKERS) {
    sum(source, 0, 0, getSize(), interpretation);
  }

  template <size_t OtherAlignment>
  void sum(
      const AlignedAudioBuffer<OtherAlignment> &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length,
      ChannelInterpretation interpretation = ChannelInterpretation::SPEAKERS) {
    if constexpr (OtherAlignment == Alignment) {
      if (&source == this) {
        return;
      }
    }

    auto numberOfSourceChannels = source.getNumberOfChannels();
    auto numberOfChannels = getNumberOfChannels();

    if (interpretation == ChannelInterpretation::DISCRETE) {
      discreteSum(source, sourceStart, destinationStart, length);
      return;
    }

    if (numberOfSourceChannels < numberOfChannels) {
      sumByUpMixing(source, sourceStart, destinationStart, length);
      return;
    }

    if (numberOfSourceChannels > numberOfChannels) {
      sumByDownMixing(source, sourceStart, destinationStart, length);
      return;
    }

    for (size_t i = 0; i < getNumberOfChannels(); ++i) {
      channels_[i]->sum(*source.channels_[i], sourceStart, destinationStart, length);
    }
  }

  /// @brief Copies audio data from a source buffer into this buffer.
  /// @note Handles up-mixing and down-mixing based on channel counts.
  template <size_t OtherAlignment>
  void copy(
      const AlignedAudioBuffer<OtherAlignment> &source) { // NOLINT(build/include_what_you_use)
    copy(source, 0, 0, getSize());
  }

  template <size_t OtherAlignment>
  void copy(
      const AlignedAudioBuffer<OtherAlignment> &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length) { // NOLINT(build/include_what_you_use)
    if constexpr (OtherAlignment == Alignment) {
      if (&source == this) {
        return;
      }
    }

    if (source.getNumberOfChannels() == getNumberOfChannels()) {
      for (size_t i = 0; i < getNumberOfChannels(); ++i) {
        channels_[i]->copy(*source.channels_[i], sourceStart, destinationStart, length);
      }
      return;
    }

    // zero + sum is equivalent to copy, but takes care of up/down-mixing.
    zero(destinationStart, length);
    sum(source, sourceStart, destinationStart, length);
  }

  /// @brief Deinterleave audio data from an interleaved source buffer.
  void deinterleaveFrom(const float *source, size_t frames) {
    if (frames == 0) {
      return;
    }

    if (numberOfChannels_ == 1) {
      channels_[0]->copy(source, 0, 0, frames);
      return;
    }

    if (numberOfChannels_ == 2) {
      dsp::deinterleaveStereo(source, channels_[0]->begin(), channels_[1]->begin(), frames);
      return;
    }

    float *channelsPtrs[MAX_CHANNEL_COUNT];
    for (size_t i = 0; i < numberOfChannels_; ++i) {
      channelsPtrs[i] = channels_[i]->begin();
    }

    for (size_t blockStart = 0; blockStart < frames; blockStart += kBlockSize) {
      size_t blockEnd = std::min(blockStart + kBlockSize, frames);
      for (size_t i = blockStart; i < blockEnd; ++i) {
        const float *frameSource = source + (i * numberOfChannels_);
        for (size_t ch = 0; ch < numberOfChannels_; ++ch) {
          channelsPtrs[ch][i] = frameSource[ch];
        }
      }
    }
  }

  /// @brief Interleave audio data from this buffer into a destination buffer.
  void interleaveTo(float *destination, size_t frames) const {
    if (frames == 0) {
      return;
    }

    if (numberOfChannels_ == 1) {
      channels_[0]->copyTo(destination, 0, 0, frames);
      return;
    }

    if (numberOfChannels_ == 2) {
      dsp::interleaveStereo(channels_[0]->begin(), channels_[1]->begin(), destination, frames);
      return;
    }

    float *channelsPtrs[MAX_CHANNEL_COUNT];
    for (size_t i = 0; i < numberOfChannels_; ++i) {
      channelsPtrs[i] = channels_[i]->begin();
    }

    for (size_t blockStart = 0; blockStart < frames; blockStart += kBlockSize) {
      size_t blockEnd = std::min(blockStart + kBlockSize, frames);
      for (size_t i = blockStart; i < blockEnd; ++i) {
        float *frameDest = destination + (i * numberOfChannels_);
        for (size_t ch = 0; ch < numberOfChannels_; ++ch) {
          frameDest[ch] = channelsPtrs[ch][i];
        }
      }
    }
  }

  void normalize() {
    float maxAbs = maxAbsValue();
    if (maxAbs == 0.0f || maxAbs == 1.0f) {
      return;
    }
    scale(1.0f / maxAbs);
  }

  void scale(float value) {
    for (auto &channel : channels_) {
      channel->scale(value);
    }
  }

  [[nodiscard]] float maxAbsValue() const {
    float maxAbs = 1.0f;
    for (const auto &channel : channels_) {
      maxAbs = std::max(maxAbs, channel->getMaxAbsValue());
    }
    return maxAbs;
  }

 private:
  std::vector<std::shared_ptr<AlignedAudioArrayBuffer<Alignment>>> channels_;

  size_t numberOfChannels_ = 0;
  float sampleRate_ = 0.0f;
  size_t size_ = 0;

  static constexpr float kSqrtHalf = 0.7071067811865476f; // sqrtf(0.5f)
  static constexpr int kBlockSize = 64;

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

  template <size_t OtherAlignment>
  void discreteSum(
      const AlignedAudioBuffer<OtherAlignment> &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length) const {
    auto numberOfChannels = std::min(getNumberOfChannels(), source.getNumberOfChannels());
    for (size_t i = 0; i < numberOfChannels; i++) {
      channels_[i]->sum(*source.getChannel(i), sourceStart, destinationStart, length);
    }
  }

  template <size_t OtherAlignment>
  void sumByUpMixing(
      const AlignedAudioBuffer<OtherAlignment> &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length) {
    auto numberOfSourceChannels = source.getNumberOfChannels();
    auto numberOfChannels = getNumberOfChannels();

    auto mix = [&](int srcType, int dstType) {
      getChannelByType(dstType)->sum(
          *source.getChannelByType(srcType), sourceStart, destinationStart, length);
    };

    // Mono to stereo (1 -> 2, 4)
    if (numberOfSourceChannels == 1 && (numberOfChannels == 2 || numberOfChannels == 4)) {
      mix(ChannelMono, ChannelLeft);
      mix(ChannelMono, ChannelRight);
    } else if (numberOfSourceChannels == 1 && numberOfChannels == 6) {
      // Mono to 5.1 (1 -> 6)
      mix(ChannelMono, ChannelCenter);
    } else if (numberOfSourceChannels == 2 && (numberOfChannels == 4 || numberOfChannels == 6)) {
      // Stereo to 4 or 5.1 (2 -> 4, 6)
      mix(ChannelLeft, ChannelLeft);
      mix(ChannelRight, ChannelRight);
    } else if (numberOfSourceChannels == 4 && numberOfChannels == 6) {
      // Stereo 4 to 5.1 (4 -> 6)
      mix(ChannelLeft, ChannelLeft);
      mix(ChannelRight, ChannelRight);
      mix(ChannelSurroundLeft, ChannelSurroundLeft);
      mix(ChannelSurroundRight, ChannelSurroundRight);
    } else {
      discreteSum(source, sourceStart, destinationStart, length);
    }
  }

  template <size_t OtherAlignment>
  void sumByDownMixing(
      const AlignedAudioBuffer<OtherAlignment> &source,
      size_t sourceStart,
      size_t destinationStart,
      size_t length) {
    auto numberOfSourceChannels = source.getNumberOfChannels();
    auto numberOfChannels = getNumberOfChannels();

    auto mix = [&](int srcType, int dstType, float gain = 1.0f) {
      getChannelByType(dstType)->sum(
          *source.getChannelByType(srcType), sourceStart, destinationStart, length, gain);
    };

    // Stereo to mono (2 -> 1): output += 0.5 * (input.L + input.R)
    if (numberOfSourceChannels == 2 && numberOfChannels == 1) {
      mix(ChannelLeft, ChannelMono, 0.5f);
      mix(ChannelRight, ChannelMono, 0.5f);
    } else if (numberOfSourceChannels == 4 && numberOfChannels == 1) {
      // Stereo 4 to mono (4 -> 1): output += 0.25 * (L + R + SL + SR)
      mix(ChannelLeft, ChannelMono, 0.25f);
      mix(ChannelRight, ChannelMono, 0.25f);
      mix(ChannelSurroundLeft, ChannelMono, 0.25f);
      mix(ChannelSurroundRight, ChannelMono, 0.25f);
    } else if (numberOfSourceChannels == 6 && numberOfChannels == 1) {
      // 5.1 to mono (6 -> 1): output += sqrt(0.5)*(L+R) + C + 0.5*(SL+SR)
      mix(ChannelLeft, ChannelMono, kSqrtHalf);
      mix(ChannelRight, ChannelMono, kSqrtHalf);
      mix(ChannelCenter, ChannelMono);
      mix(ChannelSurroundLeft, ChannelMono, 0.5f);
      mix(ChannelSurroundRight, ChannelMono, 0.5f);
    } else if (numberOfSourceChannels == 4 && numberOfChannels == 2) {
      // Stereo 4 to stereo 2 (4 -> 2): L += 0.5*(L+SL), R += 0.5*(R+SR)
      mix(ChannelLeft, ChannelLeft, 0.5f);
      mix(ChannelSurroundLeft, ChannelLeft, 0.5f);
      mix(ChannelRight, ChannelRight, 0.5f);
      mix(ChannelSurroundRight, ChannelRight, 0.5f);
    } else if (numberOfSourceChannels == 6 && numberOfChannels == 2) {
      // 5.1 to stereo (6 -> 2): L += L + sqrt(0.5)*(C+SL), R += R + sqrt(0.5)*(C+SR)
      mix(ChannelLeft, ChannelLeft);
      mix(ChannelCenter, ChannelLeft, kSqrtHalf);
      mix(ChannelSurroundLeft, ChannelLeft, kSqrtHalf);
      mix(ChannelRight, ChannelRight);
      mix(ChannelCenter, ChannelRight, kSqrtHalf);
      mix(ChannelSurroundRight, ChannelRight, kSqrtHalf);
    } else if (numberOfSourceChannels == 6 && numberOfChannels == 4) {
      // 5.1 to stereo 4 (6 -> 4): L += L + sqrt(0.5)*C, R += R + sqrt(0.5)*C, SL += SL, SR += SR
      mix(ChannelLeft, ChannelLeft);
      mix(ChannelCenter, ChannelLeft, kSqrtHalf);
      mix(ChannelRight, ChannelRight);
      mix(ChannelCenter, ChannelRight, kSqrtHalf);
      mix(ChannelSurroundLeft, ChannelSurroundLeft);
      mix(ChannelSurroundRight, ChannelSurroundRight);
    } else {
      discreteSum(source, sourceStart, destinationStart, length);
    }
  }
};

using AudioBuffer = AlignedAudioBuffer<alignof(std::max_align_t)>;
using DSPAudioBuffer = AlignedAudioBuffer<kDSPAlignment>;

} // namespace audioapi
