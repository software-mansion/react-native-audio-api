#include <algorithm>

#include "AudioArray.h"
#include "AudioBus.h"
#include "BaseAudioContext.h"
#include "Constants.h"
#include "VectorMath.h"

// Implementation of channel summing/mixing is based on the WebKit approach,
// source:
// https://github.com/WebKit/WebKit/blob/main/Source/WebCore/platform/audio/AudioBus.cpp

const float SQRT_HALF = sqrtf(0.5f);

namespace audioapi {

/**
 * Public interfaces - memory management
 */

AudioBus::AudioBus(size_t size, int numberOfChannels, float sampleRate)
    : numberOfChannels_(numberOfChannels),
      sampleRate_(sampleRate),
      size_(size) {
  createChannels();
}

AudioBus::~AudioBus() {
  channels_.clear();
}

/**
 * Public interfaces - getters
 */

int AudioBus::getNumberOfChannels() const {
  return numberOfChannels_;
}

float AudioBus::getSampleRate() const {
  return sampleRate_;
}

size_t AudioBus::getSize() const {
  return size_;
}

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

void AudioBus::sum(const AudioBus *source, ChannelInterpretation interpretation) {
  sum(source, 0, 0, getSize(), interpretation);
}

void AudioBus::sum(const AudioBus *source, size_t start, size_t length, ChannelInterpretation interpretation) {
  sum(source, start, start, length, interpretation);
}

void AudioBus::sum(
    const AudioBus *source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length,
    ChannelInterpretation interpretation) {
  if (source == this) {
    return;
  }

  int numberOfSourceChannels = source->getNumberOfChannels();
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
  for (int i = 0; i < numberOfChannels_; i += 1) {
    getChannel(i)->sum(
        source->getChannel(i), sourceStart, destinationStart, length);
  }
}

void AudioBus::copy(const AudioBus *source) {
  copy(source, 0, 0, getSize());
}

void AudioBus::copy(const AudioBus *source, size_t start, size_t length) {
  copy(source, start, start, length);
}

void AudioBus::copy(
    const AudioBus *source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  if (source == this) {
    return;
  }

  if (source->getNumberOfChannels() == getNumberOfChannels()) {
    for (int i = 0; i < getNumberOfChannels(); i += 1) {
      getChannel(i)->copy(
          source->getChannel(i), sourceStart, destinationStart, length);
    }

    return;
  }

  // zero + sum is equivalent to copy, but takes care of up/down-mixing.
  zero(destinationStart, length);
  sum(source, sourceStart, destinationStart, length);
}

/**
 * Internal tooling - channel initialization
 */

void AudioBus::createChannels() {
  channels_ = std::vector<std::shared_ptr<AudioArray>>(numberOfChannels_);

  for (int i = 0; i < numberOfChannels_; i += 1) {
    channels_[i] = std::make_shared<AudioArray>(size_);
  }
}

/**
 * Internal tooling - channel summing
 */

void AudioBus::discreteSum(
    const AudioBus *source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) const {
  int numberOfChannels =
      std::min(getNumberOfChannels(), source->getNumberOfChannels());

  // In case of source > destination, we "down-mix" and drop the extra channels.
  // In case of source < destination, we "up-mix" as many channels as we have,
  // leaving the remaining channels untouched.
  for (int i = 0; i < numberOfChannels; i++) {
    getChannel(i)->sum(
        source->getChannel(i), sourceStart, destinationStart, length);
  }
}

void AudioBus::sumByUpMixing(
    const AudioBus *source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  int numberOfSourceChannels = source->getNumberOfChannels();
  int numberOfChannels = getNumberOfChannels();

  // Mono to stereo (1 -> 2, 4)
  if (numberOfSourceChannels == 1 &&
      (numberOfChannels == 2 || numberOfChannels == 4)) {
    AudioArray *sourceChannel = source->getChannelByType(ChannelMono);

    getChannelByType(ChannelLeft)
        ->sum(sourceChannel, sourceStart, destinationStart, length);
    getChannelByType(ChannelRight)
        ->sum(sourceChannel, sourceStart, destinationStart, length);
    return;
  }

  // Mono to 5.1 (1 -> 6)
  if (numberOfSourceChannels == 1 && numberOfChannels == 6) {
    AudioArray *sourceChannel = source->getChannel(0);

    getChannelByType(ChannelCenter)
        ->sum(sourceChannel, sourceStart, destinationStart, length);
    return;
  }

  // Stereo 2 to stereo 4 or 5.1 (2 -> 4, 6)
  if (numberOfSourceChannels == 2 &&
      (numberOfChannels == 4 || numberOfChannels == 6)) {
    getChannelByType(ChannelLeft)
        ->sum(
            source->getChannelByType(ChannelLeft),
            sourceStart,
            destinationStart,
            length);
    getChannelByType(ChannelRight)
        ->sum(
            source->getChannelByType(ChannelRight),
            sourceStart,
            destinationStart,
            length);
    return;
  }

  // Stereo 4 to 5.1 (4 -> 6)
  if (numberOfSourceChannels == 4 && numberOfChannels == 6) {
    getChannelByType(ChannelLeft)
        ->sum(
            source->getChannelByType(ChannelLeft),
            sourceStart,
            destinationStart,
            length);
    getChannelByType(ChannelRight)
        ->sum(
            source->getChannelByType(ChannelRight),
            sourceStart,
            destinationStart,
            length);
    getChannelByType(ChannelSurroundLeft)
        ->sum(
            source->getChannelByType(ChannelSurroundLeft),
            sourceStart,
            destinationStart,
            length);
    getChannelByType(ChannelSurroundRight)
        ->sum(
            source->getChannelByType(ChannelSurroundRight),
            sourceStart,
            destinationStart,
            length);
    return;
  }

  discreteSum(source, sourceStart, destinationStart, length);
}

void AudioBus::sumByDownMixing(
    const AudioBus *source,
    size_t sourceStart,
    size_t destinationStart,
    size_t length) {
  int numberOfSourceChannels = source->getNumberOfChannels();
  int numberOfChannels = getNumberOfChannels();

  // Stereo to mono (2 -> 1): output += 0.5 * (input.left + input.right).
  if (numberOfSourceChannels == 2 && numberOfChannels == 1) {
    float *sourceLeft = source->getChannelByType(ChannelLeft)->getData();
    float *sourceRight = source->getChannelByType(ChannelRight)->getData();

    float *destinationData = getChannelByType(ChannelMono)->getData();

    VectorMath::multiplyByScalarThenAddToOutput(
        sourceLeft + sourceStart,
        0.5f,
        destinationData + destinationStart,
        length);
    VectorMath::multiplyByScalarThenAddToOutput(
        sourceRight + sourceStart,
        0.5f,
        destinationData + destinationStart,
        length);
    return;
  }

  // Stereo 4 to mono (4 -> 1):
  // output += 0.25 * (input.left + input.right + input.surroundLeft + input.surroundRight)
  if (numberOfSourceChannels == 4 && numberOfChannels == 1) {
    float *sourceLeft = source->getChannelByType(ChannelLeft)->getData();
    float *sourceRight = source->getChannelByType(ChannelRight)->getData();
    float *sourceSurroundLeft =
        source->getChannelByType(ChannelSurroundLeft)->getData();
    float *sourceSurroundRight =
        source->getChannelByType(ChannelSurroundRight)->getData();

    float *destinationData = getChannelByType(ChannelMono)->getData();

    VectorMath::multiplyByScalarThenAddToOutput(
        sourceLeft + sourceStart,
        0.25f,
        destinationData + destinationStart,
        length);
    VectorMath::multiplyByScalarThenAddToOutput(
        sourceRight + sourceStart,
        0.25f,
        destinationData + destinationStart,
        length);
    VectorMath::multiplyByScalarThenAddToOutput(
        sourceSurroundLeft + sourceStart,
        0.25f,
        destinationData + destinationStart,
        length);
    VectorMath::multiplyByScalarThenAddToOutput(
        sourceSurroundRight + sourceStart,
        0.25f,
        destinationData + destinationStart,
        length);
    return;
  }

  // 5.1 to mono (6 -> 1):
  // output += sqrt(1/2) * (input.left + input.right) + input.center + 0.5 * (input.surroundLeft + input.surroundRight)
  if (numberOfSourceChannels == 6 && numberOfChannels == 1) {
    float *sourceLeft = source->getChannelByType(ChannelLeft)->getData();
    float *sourceRight = source->getChannelByType(ChannelRight)->getData();
    float *sourceCenter = source->getChannelByType(ChannelCenter)->getData();
    float *sourceSurroundLeft =
          source->getChannelByType(ChannelSurroundLeft)->getData();
    float *sourceSurroundRight =
          source->getChannelByType(ChannelSurroundRight)->getData();

    float *destinationData = getChannelByType(ChannelMono)->getData();

    VectorMath::multiplyByScalarThenAddToOutput(
    sourceLeft + sourceStart,
    SQRT_HALF,
    destinationData + destinationStart,
    length);
    VectorMath::multiplyByScalarThenAddToOutput(
    sourceRight + sourceStart,
    SQRT_HALF,
    destinationData + destinationStart,
    length);
    VectorMath::add(
    sourceCenter + sourceStart,
    destinationData + destinationStart,
    destinationData + destinationStart,
    length);
    VectorMath::multiplyByScalarThenAddToOutput(
    sourceSurroundLeft + sourceStart,
    0.5f,
    destinationData + destinationStart,
    length);
    VectorMath::multiplyByScalarThenAddToOutput(
    sourceSurroundRight + sourceStart,
    0.5f,
    destinationData + destinationStart,
    length);

    return;
  }

  // Stereo 4 to stereo 2 (4 -> 2):
  // output.left += 0.5 * (input.left +  input.surroundLeft)
  // output.right += 0.5 * (input.right + input.surroundRight)
  if (numberOfSourceChannels == 4 && numberOfChannels == 2) {
    float *sourceLeft = source->getChannelByType(ChannelLeft)->getData();
    float *sourceRight = source->getChannelByType(ChannelRight)->getData();
    float *sourceSurroundLeft =
            source->getChannelByType(ChannelSurroundLeft)->getData();
    float *sourceSurroundRight =
            source->getChannelByType(ChannelSurroundRight)->getData();

    float *destinationLeft = getChannelByType(ChannelLeft)->getData();
    float *destinationRight = getChannelByType(ChannelRight)->getData();

    VectorMath::multiplyByScalarThenAddToOutput(
            sourceLeft + sourceStart,
            0.5f,
            destinationLeft + destinationStart,
            length);
    VectorMath::multiplyByScalarThenAddToOutput(
            sourceSurroundLeft + sourceStart,
            0.5f,
            destinationLeft + destinationStart,
            length);

    VectorMath::multiplyByScalarThenAddToOutput(
            sourceRight + sourceStart,
            0.5f,
            destinationRight + destinationStart,
            length);
    VectorMath::multiplyByScalarThenAddToOutput(
            sourceSurroundRight + sourceStart,
            0.5f,
            destinationRight + destinationStart,
            length);
    return;
  }

  // 5.1 to stereo (6 -> 2):
  // output.left += input.left + sqrt(1/2) * (input.center + input.surroundLeft)
  // output.right += input.right + sqrt(1/2) * (input.center + input.surroundRight)
  if (numberOfSourceChannels == 6 && numberOfChannels == 2) {
    float *sourceLeft = source->getChannelByType(ChannelLeft)->getData();
    float *sourceRight = source->getChannelByType(ChannelRight)->getData();
    float *sourceCenter = source->getChannelByType(ChannelCenter)->getData();
    float *sourceSurroundLeft =
        source->getChannelByType(ChannelSurroundLeft)->getData();
    float *sourceSurroundRight =
        source->getChannelByType(ChannelSurroundRight)->getData();

    float *destinationLeft = getChannelByType(ChannelLeft)->getData();
    float *destinationRight = getChannelByType(ChannelRight)->getData();

    VectorMath::add(
        sourceLeft + sourceStart,
        destinationLeft + destinationStart,
        destinationLeft + destinationStart,
        length);
    VectorMath::multiplyByScalarThenAddToOutput(
        sourceCenter + sourceStart,
        SQRT_HALF,
        destinationLeft + destinationStart,
        length);
    VectorMath::multiplyByScalarThenAddToOutput(
        sourceSurroundLeft + sourceStart,
        SQRT_HALF,
        destinationLeft + destinationStart,
        length);

    VectorMath::add(
        sourceRight + sourceStart,
        destinationRight + destinationStart,
        destinationRight + destinationStart,
        length);
    VectorMath::multiplyByScalarThenAddToOutput(
        sourceCenter + sourceStart,
        SQRT_HALF,
        destinationRight + destinationStart,
        length);
    VectorMath::multiplyByScalarThenAddToOutput(
        sourceSurroundRight + sourceStart,
        SQRT_HALF,
        destinationRight + destinationStart,
        length);
    return;
  }

  // 5.1 to stereo 4 (6 -> 4):
  // output.left += input.left + sqrt(1/2) * input.center
  // output.right += input.right + sqrt(1/2) * input.center
  // output.surroundLeft += input.surroundLeft
  // output.surroundRight += input.surroundRight
  if (numberOfSourceChannels == 6 && numberOfChannels == 4) {
    float *sourceLeft = source->getChannelByType(ChannelLeft)->getData();
    float *sourceRight = source->getChannelByType(ChannelRight)->getData();
    float *sourceCenter = source->getChannelByType(ChannelCenter)->getData();
    float *sourceSurroundLeft =
        source->getChannelByType(ChannelSurroundLeft)->getData();
    float *sourceSurroundRight =
        source->getChannelByType(ChannelSurroundRight)->getData();

    float *destinationLeft = getChannelByType(ChannelLeft)->getData();
    float *destinationRight = getChannelByType(ChannelRight)->getData();
    float *destinationSurroundLeft =
        getChannelByType(ChannelSurroundLeft)->getData();
    float *destinationSurroundRight =
        getChannelByType(ChannelSurroundRight)->getData();

    VectorMath::add(
        sourceLeft + sourceStart,
        destinationLeft + destinationStart,
        destinationLeft + destinationStart,
        length);
    VectorMath::multiplyByScalarThenAddToOutput(
        sourceCenter, SQRT_HALF, destinationLeft + destinationStart, length);

    VectorMath::add(
        sourceRight + sourceStart,
        destinationRight + destinationStart,
        destinationRight + destinationStart,
        length);
    VectorMath::multiplyByScalarThenAddToOutput(
        sourceCenter, SQRT_HALF, destinationRight + destinationStart, length);

    VectorMath::add(
        sourceSurroundLeft + sourceStart,
        destinationSurroundLeft + destinationStart,
        destinationSurroundLeft + destinationStart,
        length);
    VectorMath::add(
        sourceSurroundRight + sourceStart,
        destinationSurroundRight + destinationStart,
        destinationSurroundRight + destinationStart,
        length);
    return;
  }

  discreteSum(source, sourceStart, destinationStart, length);
}

} // namespace audioapi
