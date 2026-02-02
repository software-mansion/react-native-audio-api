#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

// Implementation of channel summing/mixing is based on the WebKit approach,
// source:
// https://github.com/WebKit/WebKit/blob/main/Source/WebCore/platform/audio/AudioBus.cpp

const float SQRT_HALF = sqrtf(0.5f);

namespace audioapi {

/**
 * Public interfaces - memory management
 */

AudioBus::AudioBus(size_t size, int numberOfChannels, float sampleRate)
    : numberOfChannels_(numberOfChannels), sampleRate_(sampleRate), size_(size) {
  createChannels();
}

AudioBus::AudioBus(const AudioBus &other): numberOfChannels_(other.numberOfChannels_),
                                           sampleRate_(other.sampleRate_),
                                           size_(other.size_) {
  createChannels();

  for (int i = 0; i < numberOfChannels_; i += 1) {
    *channels_[i] = *other.channels_[i];
  }
}

AudioBus::AudioBus(audioapi::AudioBus &&other) noexcept :
  channels_(std::move(other.channels_)),
  numberOfChannels_(other.numberOfChannels_),
  sampleRate_(other.sampleRate_),
  size_(other.size_) {
    other.numberOfChannels_ = 0;
    other.sampleRate_ = 0.0f;
    other.size_ = 0;
}

AudioBus &AudioBus::operator=(const AudioBus &other) {
  if (this != &other) {
      if (numberOfChannels_ != other.numberOfChannels_ || size_ != other.size_) {
          numberOfChannels_ = other.numberOfChannels_;
          size_ = other.size_;
          createChannels();
      }

      sampleRate_ = other.sampleRate_;

      for (int i = 0; i < numberOfChannels_; i += 1) {
          *channels_[i] = *other.channels_[i];
      }
  }

  return *this;
}

AudioBus &AudioBus::operator=(audioapi::AudioBus &&other) noexcept {
    if (this != &other) {
      channels_ = std::move(other.channels_);

      numberOfChannels_ = other.numberOfChannels_;
      sampleRate_ = other.sampleRate_;
      size_ = other.size_;

      other.numberOfChannels_ = 0;
      other.sampleRate_ = 0.0f;
      other.size_ = 0;
    }
    return *this;
}

/**
 * Public interfaces - getters
 */

AudioArray *AudioBus::getChannel(int index) const {
  return channels_[index].get();
}

AudioArray *AudioBus::getChannelByType(int channelType) const {
  switch (getNumberOfChannels()) {
    case 1: // mono
      if (channelType == ChannelMono) {
        return getChannel(0);
      }
      return nullptr;

    case 2: // stereo
      switch (channelType) {
        case ChannelLeft:
          return getChannel(0);
        case ChannelRight:
          return getChannel(1);
        default:
          return nullptr;
      }

    case 4: // quad
      switch (channelType) {
        case ChannelLeft:
          return getChannel(0);
        case ChannelRight:
          return getChannel(1);
        case ChannelSurroundLeft:
          return getChannel(2);
        case ChannelSurroundRight:
          return getChannel(3);
        default:
          return nullptr;
      }

    case 5: // 5.0
      switch (channelType) {
        case ChannelLeft:
          return getChannel(0);
        case ChannelRight:
          return getChannel(1);
        case ChannelCenter:
          return getChannel(2);
        case ChannelSurroundLeft:
          return getChannel(3);
        case ChannelSurroundRight:
          return getChannel(4);
        default:
          return nullptr;
      }

    case 6: // 5.1
      switch (channelType) {
        case ChannelLeft:
          return getChannel(0);
        case ChannelRight:
          return getChannel(1);
        case ChannelCenter:
          return getChannel(2);
        case ChannelLFE:
          return getChannel(3);
        case ChannelSurroundLeft:
          return getChannel(4);
        case ChannelSurroundRight:
          return getChannel(5);
        default:
          return nullptr;
      }
    default:
      return nullptr;
  }
}

std::shared_ptr<AudioArray> AudioBus::getSharedChannel(int index) const {
  return channels_[index];
}

/**
 * Public interfaces - audio processing and setters
 */

void AudioBus::zero() {
  zero(0, getSize());
}

void AudioBus::zero(size_t start, size_t length) {
  for (auto it = channels_.begin(); it != channels_.end(); it += 1) {
    it->get()->zero(start, length);
  }
}

void AudioBus::sum(const AudioBus& source, ChannelInterpretation interpretation) {
  sum(source, 0, 0, getSize(), interpretation);
}

void AudioBus::sum(
    const AudioBus& source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length,
    ChannelInterpretation interpretation) {
  if (&source == this) {
    return;
  }

  int numberOfSourceChannels = source.getNumberOfChannels();
  int numberOfChannels = getNumberOfChannels();

  if (interpretation == ChannelInterpretation::DISCRETE) {
    discreteSum(source, sourceStart, destinationStart, length);
    return;
  }

  // Source channel count is smaller than current bus, we need to up-mix.
  if (numberOfSourceChannels < numberOfChannels) {
    sumByUpMixing(source, sourceStart, destinationStart, length);
    return;
  }

  // Source channel count is larger than current bus, we need to down-mix.
  if (numberOfSourceChannels > numberOfChannels) {
    sumByDownMixing(source, sourceStart, destinationStart, length);
    return;
  }

  // Source and destination channel counts are the same. Just sum the channels.
  for (int i = 0; i < getNumberOfChannels(); i += 1) {
    channels_[i]->sum(*source.channels_[i], sourceStart, destinationStart, length);
  }
}

void AudioBus::copy(const AudioBus& source) {
  copy(source, 0, 0, getSize());
}

void AudioBus::copy(
    const AudioBus& source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  if (&source == this) {
    return;
  }

  if (source.getNumberOfChannels() == getNumberOfChannels()) {
    for (int i = 0; i < getNumberOfChannels(); i += 1) {
      channels_[i]->copy(*source.channels_[i], sourceStart, destinationStart, length);
    }

    return;
  }

  // zero + sum is equivalent to copy, but takes care of up/down-mixing.
  zero(destinationStart, length);
  sum(source, sourceStart, destinationStart, length);
}

void AudioBus::normalize() {
    float maxAbsValue = this->maxAbsValue();

    if (maxAbsValue == 0.0f || maxAbsValue == 1.0f) {
        return;
    }

    float scale = 1.0f / maxAbsValue;
    this->scale(scale);
}

void AudioBus::scale(float value) {
    for (auto &channel : channels_) {
        channel->scale(value);
    }
}

float AudioBus::maxAbsValue() const {
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

void AudioBus::createChannels() {
  if (channels_.size() != static_cast<size_t>(numberOfChannels_)) {
      channels_.clear();
      channels_.reserve(numberOfChannels_);

      for (int i = 0; i < numberOfChannels_; i += 1) {
        channels_.emplace_back(std::make_shared<AudioArray>(size_));
      }
  } else {
      for (int i = 0; i < numberOfChannels_; i += 1) {
        channels_[i]->resize(size_);
      }
  }
}

/**
 * Internal tooling - channel summing
 */

void AudioBus::discreteSum(
    const AudioBus& source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) const {
  int numberOfChannels = std::min(getNumberOfChannels(), source.getNumberOfChannels());

  // In case of source > destination, we "down-mix" and drop the extra channels.
  // In case of source < destination, we "up-mix" as many channels as we have,
  // leaving the remaining channels untouched.
  for (int i = 0; i < numberOfChannels; i++) {
    channels_[i]->sum(*source.channels_[i], sourceStart, destinationStart, length);
  }
}

void AudioBus::sumByUpMixing(
    const AudioBus& source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  int numberOfSourceChannels = source.getNumberOfChannels();
  int numberOfChannels = getNumberOfChannels();

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
        ->sum(*source.getChannelByType(ChannelSurroundRight), sourceStart, destinationStart, length);
    return;
  }

  discreteSum(source, sourceStart, destinationStart, length);
}

void AudioBus::sumByDownMixing(
    const AudioBus& source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  int numberOfSourceChannels = source.getNumberOfChannels();
  int numberOfChannels = getNumberOfChannels();

  // Stereo to mono (2 -> 1): output += 0.5 * (input.left + input.right).
  if (numberOfSourceChannels == 2 && numberOfChannels == 1) {
    auto destinationData = getChannelByType(ChannelMono);

    destinationData->sum(*source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length, 0.5f);
    destinationData->sum(*source.getChannelByType(ChannelRight), sourceStart, destinationStart, length, 0.5f);
    return;
  }

  // Stereo 4 to mono (4 -> 1):
  // output += 0.25 * (input.left + input.right + input.surroundLeft +
  // input.surroundRight)
  if (numberOfSourceChannels == 4 && numberOfChannels == 1) {
    auto destinationData = getChannelByType(ChannelMono);

    destinationData->sum(*source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length, 0.25f);
    destinationData->sum(*source.getChannelByType(ChannelRight), sourceStart, destinationStart, length, 0.25f);
    destinationData->sum(*source.getChannelByType(ChannelSurroundLeft), sourceStart, destinationStart, length, 0.25f);
    destinationData->sum(*source.getChannelByType(ChannelSurroundRight), sourceStart, destinationStart, length, 0.25f);
    return;
  }

  // 5.1 to mono (6 -> 1):
  // output += sqrt(1/2) * (input.left + input.right) + input.center + 0.5 *
  // (input.surroundLeft + input.surroundRight)
  if (numberOfSourceChannels == 6 && numberOfChannels == 1) {
    auto destinationData = getChannelByType(ChannelMono);

    destinationData->sum(*source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length, SQRT_HALF);
    destinationData->sum(*source.getChannelByType(ChannelRight), sourceStart, destinationStart, length, SQRT_HALF);
    destinationData->sum(*source.getChannelByType(ChannelCenter), sourceStart, destinationStart, length);
    destinationData->sum(*source.getChannelByType(ChannelSurroundLeft), sourceStart, destinationStart, length, 0.5f);
    destinationData->sum(*source.getChannelByType(ChannelSurroundRight), sourceStart, destinationStart, length, 0.5f);
    return;
  }

  // Stereo 4 to stereo 2 (4 -> 2):
  // output.left += 0.5 * (input.left +  input.surroundLeft)
  // output.right += 0.5 * (input.right + input.surroundRight)
  if (numberOfSourceChannels == 4 && numberOfChannels == 2) {
    auto destinationLeft = getChannelByType(ChannelLeft);
    auto destinationRight = getChannelByType(ChannelRight);

    destinationLeft->sum(*source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length, 0.5f);
    destinationLeft->sum(*source.getChannelByType(ChannelSurroundLeft), sourceStart, destinationStart, length, 0.5f);

    destinationRight->sum(*source.getChannelByType(ChannelRight), sourceStart, destinationStart, length, 0.5f);
    destinationRight->sum(*source.getChannelByType(ChannelSurroundRight), sourceStart, destinationStart, length, 0.5f);
    return;
  }

  // 5.1 to stereo (6 -> 2):
  // output.left += input.left + sqrt(1/2) * (input.center + input.surroundLeft)
  // output.right += input.right + sqrt(1/2) * (input.center +
  // input.surroundRight)
  if (numberOfSourceChannels == 6 && numberOfChannels == 2) {
    auto destinationLeft = getChannelByType(ChannelLeft);
    auto destinationRight = getChannelByType(ChannelRight);

    destinationLeft->sum(*source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length);
    destinationLeft->sum(*source.getChannelByType(ChannelCenter), sourceStart, destinationStart, length, SQRT_HALF);
    destinationLeft->sum(*source.getChannelByType(ChannelSurroundLeft), sourceStart, destinationStart, length, SQRT_HALF);

    destinationRight->sum(*source.getChannelByType(ChannelRight), sourceStart, destinationStart, length);
    destinationRight->sum(*source.getChannelByType(ChannelCenter), sourceStart, destinationStart, length, SQRT_HALF);
    destinationRight->sum(*source.getChannelByType(ChannelSurroundRight), sourceStart, destinationStart, length, SQRT_HALF);
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

    destinationLeft->sum(*source.getChannelByType(ChannelLeft), sourceStart, destinationStart, length);
    destinationLeft->sum(*source.getChannelByType(ChannelCenter), sourceStart, destinationStart, length, SQRT_HALF);

    destinationRight->sum(*source.getChannelByType(ChannelRight), sourceStart, destinationStart, length);
    destinationRight->sum(*source.getChannelByType(ChannelCenter), sourceStart, destinationStart, length, SQRT_HALF);

    destinationSurroundLeft->sum(*source.getChannelByType(ChannelSurroundLeft), sourceStart, destinationStart, length);
    destinationSurroundRight->sum(*source.getChannelByType(ChannelSurroundRight), sourceStart, destinationStart, length);
    return;
  }

  discreteSum(source, sourceStart, destinationStart, length);
}

} // namespace audioapi
