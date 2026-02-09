#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioArrayBuffer.hpp>
#include <audioapi/utils/AudioBuffer.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

const float SQRT_HALF = sqrtf(0.5f);
constexpr int BLOCK_SIZE = 64;

AudioBuffer::AudioBuffer(size_t size, int numberOfChannels, float sampleRate)
    : numberOfChannels_(numberOfChannels), sampleRate_(sampleRate), size_(size) {
  channels_.reserve(numberOfChannels_);
  for (size_t i = 0; i < numberOfChannels_; ++i) {
    channels_.emplace_back(std::make_shared<AudioArrayBuffer>(size_));
  }
}

AudioBuffer::AudioBuffer(const AudioBuffer &other)
    : numberOfChannels_(other.numberOfChannels_),
      sampleRate_(other.sampleRate_),
      size_(other.size_) {
  channels_.reserve(numberOfChannels_);
  for (const auto& channel : other.channels_) {
    channels_.emplace_back(std::make_shared<AudioArrayBuffer>(*channel));
  }
}

AudioBuffer::AudioBuffer(audioapi::AudioBuffer &&other) noexcept
    : channels_(std::move(other.channels_)),
      numberOfChannels_(std::exchange(other.numberOfChannels_, 0)),
      sampleRate_(std::exchange(other.sampleRate_, 0.0f)),
      size_(std::exchange(other.size_, 0)) {}

AudioBuffer &AudioBuffer::operator=(const AudioBuffer &other) {
  if (this != &other) {
    sampleRate_ = other.sampleRate_;

    if (numberOfChannels_ != other.numberOfChannels_) {
      numberOfChannels_ = other.numberOfChannels_;
      size_ = other.size_;
      channels_.clear();
      channels_.reserve(numberOfChannels_);

      for (const auto& channel : other.channels_) {
        channels_.emplace_back(std::make_shared<AudioArrayBuffer>(*channel));
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

AudioBuffer &AudioBuffer::operator=(audioapi::AudioBuffer &&other) noexcept {
  if (this != &other) {
    channels_ = std::move(other.channels_);

    numberOfChannels_ = std::exchange(other.numberOfChannels_, 0);
    sampleRate_ = std::exchange(other.sampleRate_, 0.0f);
    size_ = std::exchange(other.size_, 0);
  }
  return *this;
}

AudioArray *AudioBuffer::getChannel(size_t index) const {
  return channels_[index].get();
}

AudioArray *AudioBuffer::getChannelByType(int channelType) const {
    auto it = kChannelLayouts.find(getNumberOfChannels());
    if (it == kChannelLayouts.end()) {
        return nullptr;
    }
    const auto& channelOrder = it->second;
    for (size_t i = 0; i < channelOrder.size(); ++i) {
        if (channelOrder[i] == channelType) {
            return getChannel(i);
        }
    }

    return nullptr;
}

std::shared_ptr<AudioArrayBuffer> AudioBuffer::getSharedChannel(size_t index) const {
  return channels_[index];
}

void AudioBuffer::zero() {
  zero(0, getSize());
}

void AudioBuffer::zero(size_t start, size_t length) {
  for (auto it = channels_.begin(); it != channels_.end(); it += 1) {
    it->get()->zero(start, length);
  }
}

void AudioBuffer::sum(const AudioBuffer &source, ChannelInterpretation interpretation) {
  sum(source, 0, 0, getSize(), interpretation);
}

void AudioBuffer::sum(
    const AudioBuffer &source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length,
    ChannelInterpretation interpretation) {
  if (&source == this) {
    return;
  }

  auto numberOfSourceChannels = source.getNumberOfChannels();
  auto numberOfChannels = getNumberOfChannels();

  if (interpretation == ChannelInterpretation::DISCRETE) {
    discreteSum(source, sourceStart, destinationStart, length);
    return;
  }

  // Source channel count is smaller than current buffer, we need to up-mix.
  if (numberOfSourceChannels < numberOfChannels) {
    sumByUpMixing(source, sourceStart, destinationStart, length);
    return;
  }

  // Source channel count is larger than current buffer, we need to down-mix.
  if (numberOfSourceChannels > numberOfChannels) {
    sumByDownMixing(source, sourceStart, destinationStart, length);
    return;
  }

  // Source and destination channel counts are the same. Just sum the channels.
  for (size_t i = 0; i < getNumberOfChannels(); ++i) {
    channels_[i]->sum(*source.channels_[i], sourceStart, destinationStart, length);
  }
}

void AudioBuffer::copy(const AudioBuffer &source) {
  copy(source, 0, 0, getSize());
}

void AudioBuffer::copy(
    const AudioBuffer &source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  if (&source == this) {
    return;
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

void AudioBuffer::deinterleaveFrom(const float *source, size_t frames) {
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

  for (size_t blockStart = 0; blockStart < frames; blockStart += BLOCK_SIZE) {
    size_t blockEnd = std::min(blockStart + BLOCK_SIZE, frames);
    for (size_t i = blockStart; i < blockEnd; ++i) {
      const float *frameSource = source + (i * numberOfChannels_);
      for (size_t ch = 0; ch < numberOfChannels_; ++ch) {
        channelsPtrs[ch][i] = frameSource[ch];
      }
    }
  }
}

void AudioBuffer::interleaveTo(float *destination, size_t frames) const {
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

  for (size_t blockStart = 0; blockStart < frames; blockStart += BLOCK_SIZE) {
    size_t blockEnd = std::min(blockStart + BLOCK_SIZE, frames);
    for (size_t i = blockStart; i < blockEnd; ++i) {
      float *frameDest = destination + (i * numberOfChannels_);
      for (size_t ch = 0; ch < numberOfChannels_; ++ch) {
        frameDest[ch] = channelsPtrs[ch][i];
      }
    }
  }
}

void AudioBuffer::normalize() {
  float maxAbsValue = this->maxAbsValue();

  if (maxAbsValue == 0.0f || maxAbsValue == 1.0f) {
    return;
  }

  float scale = 1.0f / maxAbsValue;
  this->scale(scale);
}

void AudioBuffer::scale(float value) {
  for (auto &channel : channels_) {
    channel->scale(value);
  }
}

float AudioBuffer::maxAbsValue() const {
  float maxAbsValue = 1.0f;

  for (const auto &channel : channels_) {
    float channelMaxAbsValue = channel->getMaxAbsValue();
    maxAbsValue = std::max(maxAbsValue, channelMaxAbsValue);
  }

  return maxAbsValue;
}

/**
 * Internal tooling - channel initialization
 */

void AudioBuffer::discreteSum(
    const AudioBuffer &source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) const {
  auto numberOfChannels = std::min(getNumberOfChannels(), source.getNumberOfChannels());

  // In case of source > destination, we "down-mix" and drop the extra channels.
  // In case of source < destination, we "up-mix" as many channels as we have,
  // leaving the remaining channels untouched.
  for (size_t i = 0; i < numberOfChannels; i++) {
    channels_[i]->sum(*source.channels_[i], sourceStart, destinationStart, length);
  }
}

void AudioBuffer::sumByUpMixing(
    const AudioBuffer &source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  auto numberOfSourceChannels = source.getNumberOfChannels();
  auto numberOfChannels = getNumberOfChannels();

  // Mono to stereo (1 -> 2, 4)
  if (numberOfSourceChannels == 1 && (numberOfChannels == 2 || numberOfChannels == 4)) {
    AudioArray *sourceChannel = source.getChannelByType(ChannelMono);

    getChannelByType(ChannelLeft)->sum(*sourceChannel, sourceStart, destinationStart, length);
    getChannelByType(ChannelRight)->sum(*sourceChannel, sourceStart, destinationStart, length);
    return;
  }

  // Mono to 5.1 (1 -> 6)
  if (numberOfSourceChannels == 1 && numberOfChannels == 6) {
    AudioArray *sourceChannel = source.getChannel(0);

    getChannelByType(ChannelCenter)->sum(*sourceChannel, sourceStart, destinationStart, length);
    return;
  }

  // Stereo 2 to stereo 4 or 5.1 (2 -> 4, 6)
  if (numberOfSourceChannels == 2 && (numberOfChannels == 4 || numberOfChannels == 6)) {
    getChannelByType(ChannelLeft)
        ->sum(*source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length);
    getChannelByType(ChannelRight)
        ->sum(*source.getChannelByType(ChannelRight), sourceStart, destinationStart, length);
    return;
  }

  // Stereo 4 to 5.1 (4 -> 6)
  if (numberOfSourceChannels == 4 && numberOfChannels == 6) {
    getChannelByType(ChannelLeft)
        ->sum(*source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length);
    getChannelByType(ChannelRight)
        ->sum(*source.getChannelByType(ChannelRight), sourceStart, destinationStart, length);
    getChannelByType(ChannelSurroundLeft)
        ->sum(*source.getChannelByType(ChannelSurroundLeft), sourceStart, destinationStart, length);
    getChannelByType(ChannelSurroundRight)
        ->sum(
            *source.getChannelByType(ChannelSurroundRight), sourceStart, destinationStart, length);
    return;
  }

  discreteSum(source, sourceStart, destinationStart, length);
}

void AudioBuffer::sumByDownMixing(
    const AudioBuffer &source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  auto numberOfSourceChannels = source.getNumberOfChannels();
  auto numberOfChannels = getNumberOfChannels();

  // Stereo to mono (2 -> 1): output += 0.5 * (input.left + input.right).
  if (numberOfSourceChannels == 2 && numberOfChannels == 1) {
    auto destinationData = getChannelByType(ChannelMono);

    destinationData->sum(
        *source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length, 0.5f);
    destinationData->sum(
        *source.getChannelByType(ChannelRight), sourceStart, destinationStart, length, 0.5f);
    return;
  }

  // Stereo 4 to mono (4 -> 1):
  // output += 0.25 * (input.left + input.right + input.surroundLeft +
  // input.surroundRight)
  if (numberOfSourceChannels == 4 && numberOfChannels == 1) {
    auto destinationData = getChannelByType(ChannelMono);

    destinationData->sum(
        *source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length, 0.25f);
    destinationData->sum(
        *source.getChannelByType(ChannelRight), sourceStart, destinationStart, length, 0.25f);
    destinationData->sum(
        *source.getChannelByType(ChannelSurroundLeft),
        sourceStart,
        destinationStart,
        length,
        0.25f);
    destinationData->sum(
        *source.getChannelByType(ChannelSurroundRight),
        sourceStart,
        destinationStart,
        length,
        0.25f);
    return;
  }

  // 5.1 to mono (6 -> 1):
  // output += sqrt(1/2) * (input.left + input.right) + input.center + 0.5 *
  // (input.surroundLeft + input.surroundRight)
  if (numberOfSourceChannels == 6 && numberOfChannels == 1) {
    auto destinationData = getChannelByType(ChannelMono);

    destinationData->sum(
        *source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length, SQRT_HALF);
    destinationData->sum(
        *source.getChannelByType(ChannelRight), sourceStart, destinationStart, length, SQRT_HALF);
    destinationData->sum(
        *source.getChannelByType(ChannelCenter), sourceStart, destinationStart, length);
    destinationData->sum(
        *source.getChannelByType(ChannelSurroundLeft), sourceStart, destinationStart, length, 0.5f);
    destinationData->sum(
        *source.getChannelByType(ChannelSurroundRight),
        sourceStart,
        destinationStart,
        length,
        0.5f);
    return;
  }

  // Stereo 4 to stereo 2 (4 -> 2):
  // output.left += 0.5 * (input.left +  input.surroundLeft)
  // output.right += 0.5 * (input.right + input.surroundRight)
  if (numberOfSourceChannels == 4 && numberOfChannels == 2) {
    auto destinationLeft = getChannelByType(ChannelLeft);
    auto destinationRight = getChannelByType(ChannelRight);

    destinationLeft->sum(
        *source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length, 0.5f);
    destinationLeft->sum(
        *source.getChannelByType(ChannelSurroundLeft), sourceStart, destinationStart, length, 0.5f);

    destinationRight->sum(
        *source.getChannelByType(ChannelRight), sourceStart, destinationStart, length, 0.5f);
    destinationRight->sum(
        *source.getChannelByType(ChannelSurroundRight),
        sourceStart,
        destinationStart,
        length,
        0.5f);
    return;
  }

  // 5.1 to stereo (6 -> 2):
  // output.left += input.left + sqrt(1/2) * (input.center + input.surroundLeft)
  // output.right += input.right + sqrt(1/2) * (input.center +
  // input.surroundRight)
  if (numberOfSourceChannels == 6 && numberOfChannels == 2) {
    auto destinationLeft = getChannelByType(ChannelLeft);
    auto destinationRight = getChannelByType(ChannelRight);

    destinationLeft->sum(
        *source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length);
    destinationLeft->sum(
        *source.getChannelByType(ChannelCenter), sourceStart, destinationStart, length, SQRT_HALF);
    destinationLeft->sum(
        *source.getChannelByType(ChannelSurroundLeft),
        sourceStart,
        destinationStart,
        length,
        SQRT_HALF);

    destinationRight->sum(
        *source.getChannelByType(ChannelRight), sourceStart, destinationStart, length);
    destinationRight->sum(
        *source.getChannelByType(ChannelCenter), sourceStart, destinationStart, length, SQRT_HALF);
    destinationRight->sum(
        *source.getChannelByType(ChannelSurroundRight),
        sourceStart,
        destinationStart,
        length,
        SQRT_HALF);
    return;
  }

  // 5.1 to stereo 4 (6 -> 4):
  // output.left += input.left + sqrt(1/2) * input.center
  // output.right += input.right + sqrt(1/2) * input.center
  // output.surroundLeft += input.surroundLeft
  // output.surroundRight += input.surroundRight
  if (numberOfSourceChannels == 6 && numberOfChannels == 4) {
    auto destinationLeft = getChannelByType(ChannelLeft);
    auto destinationRight = getChannelByType(ChannelRight);
    auto destinationSurroundLeft = getChannelByType(ChannelSurroundLeft);
    auto destinationSurroundRight = getChannelByType(ChannelSurroundRight);

    destinationLeft->sum(
        *source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length);
    destinationLeft->sum(
        *source.getChannelByType(ChannelCenter), sourceStart, destinationStart, length, SQRT_HALF);

    destinationRight->sum(
        *source.getChannelByType(ChannelRight), sourceStart, destinationStart, length);
    destinationRight->sum(
        *source.getChannelByType(ChannelCenter), sourceStart, destinationStart, length, SQRT_HALF);

    destinationSurroundLeft->sum(
        *source.getChannelByType(ChannelSurroundLeft), sourceStart, destinationStart, length);
    destinationSurroundRight->sum(
        *source.getChannelByType(ChannelSurroundRight), sourceStart, destinationStart, length);
    return;
  }

  discreteSum(source, sourceStart, destinationStart, length);
}

} // namespace audioapi
