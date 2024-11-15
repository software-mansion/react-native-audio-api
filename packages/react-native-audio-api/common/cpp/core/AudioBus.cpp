
#include "AudioBus.h"
#include "Constants.h"
#include "AudioArray.h"
#include "VectorMath.h"
#include "BaseAudioContext.h"

// Implementation of channel summing/mixing is based on the WebKit approach, source:
// https://github.com/WebKit/WebKit/blob/main/Source/WebCore/platform/audio/AudioBus.cpp
const float SQRT_HALF = sqrtf(0.5f);

namespace audioapi {

AudioBus::AudioBus(BaseAudioContext *context)
    : context_(context), numberOfChannels_(CHANNEL_COUNT) {
  sampleRate_ = context_->getSampleRate();
  size_ = context_->getBufferSizeInFrames();


  createChannels();
};

AudioBus::AudioBus(BaseAudioContext *context, int numberOfChannels)
    : context_(context), numberOfChannels_(numberOfChannels) {
  sampleRate_ = context_->getSampleRate();
  size_ = context_->getBufferSizeInFrames();

  createChannels();
};

AudioBus::AudioBus(BaseAudioContext *context, int numberOfChannels, int size)
    : context_(context), numberOfChannels_(numberOfChannels), size_(size) {
  sampleRate_ = context_->getSampleRate();

  createChannels();
};


AudioBus::~AudioBus() {
  channels_.clear();
};

void AudioBus::createChannels() {
  channels_ = std::vector<std::unique_ptr<AudioArray>>(numberOfChannels_, std::make_unique<AudioArray>(size_));
};

int AudioBus::getNumberOfChannels() const {
  return numberOfChannels_;
};

int AudioBus::getSampleRate() const {
  return sampleRate_;
};

int AudioBus::getSize() const {
  return size_;
};

AudioArray* AudioBus::getChannel(int index) const {
  return channels_[index].get();
};

AudioArray* AudioBus::getChannelByType(int channelType) const {
  switch (getNumberOfChannels()) {
    case 1: // mono
      if (channelType == ChannelMono || channelType == ChannelLeft) {
        return getChannel(0);
      }
      return 0;

    case 2: // stereo
      switch (channelType) {
        case ChannelLeft: return getChannel(0);
        case ChannelRight: return getChannel(1);
        default: return 0;
      }

    case 4: // quad
      switch (channelType) {
        case ChannelLeft: return getChannel(0);
        case ChannelRight: return getChannel(1);
        case ChannelSurroundLeft: return getChannel(2);
        case ChannelSurroundRight: return getChannel(3);
        default: return 0;
      }

    case 5: // 5.0
      switch (channelType) {
        case ChannelLeft: return getChannel(0);
        case ChannelRight: return getChannel(1);
        case ChannelCenter: return getChannel(2);
        case ChannelSurroundLeft: return getChannel(3);
        case ChannelSurroundRight: return getChannel(4);
        default: return 0;
      }

    case 6: // 5.1
      switch (channelType) {
        case ChannelLeft: return getChannel(0);
        case ChannelRight: return getChannel(1);
        case ChannelCenter: return getChannel(2);
        case ChannelLFE: return getChannel(3);
        case ChannelSurroundLeft: return getChannel(4);
        case ChannelSurroundRight: return getChannel(5);
        default: return 0;
      }
    default:
      return 0;
  }
};

void AudioBus::zero() {
  for (auto it = channels_.begin(); it != channels_.end(); it += 1) {
    it->get()->zero();
  }
};

void AudioBus::normalize() {
  float maxAbsValue = this->maxAbsValue();

  if (maxAbsValue == 0.0f || maxAbsValue == 1.0f) {
    return;
  }

  float scale = 1.0f / maxAbsValue;
  this->scale(scale);
};

void AudioBus::scale(float value) {
  for (auto it = channels_.begin(); it != channels_.end(); it += 1) {
    it->get()->scale(value);
  }
};

float AudioBus::maxAbsValue() const {
  float maxAbsValue = 0.0f;

  for (auto it = channels_.begin(); it != channels_.end(); it += 1) {
    float channelMaxAbsValue = it->get()->getMaxAbsValue();
    maxAbsValue = std::max(maxAbsValue, channelMaxAbsValue);
  }

  return maxAbsValue;
};

void AudioBus::copy(const AudioBus &source) {
  if (&source == this) {
    return;
  }

  zero();
  sum(source);
};

void AudioBus::sum(const AudioBus &source) {
  if (&source == this) {
    return;
  }

  int numberOfSourceChannels = source.getNumberOfChannels();
  int numberOfChannels = getNumberOfChannels();

  // TODO: consider adding ability to enforce discrete summing (if/when it will be useful).
  if (numberOfSourceChannels < numberOfChannels) {
    sumByDownMixing(source);
    return;
  }

  if (numberOfSourceChannels > numberOfChannels) {
    sumByUpMixing(source);
    return;
  }

  for (int i = 0; i < numberOfChannels_; i++) {
    channels_[i]->sum(source.getChannel(i));
  }
};

void AudioBus::discreteSum(const AudioBus &source) {
  int numberOfChannels = std::min(getNumberOfChannels(), source.getNumberOfChannels());

  // In case of source > destination, we "down-mix" and drop the extra channels.
  // In case of source < destination, we "up-mix" as many channels as we have, leaving the remaining channels untouched.
  for (int i = 0; i < numberOfChannels; i++) {
    getChannel(i)->sum(source.getChannel(i));
  }
};

void AudioBus::sumByUpMixing(const AudioBus &source) {
  int numberOfSourceChannels = source.getNumberOfChannels();
  int numberOfChannels = getNumberOfChannels();

  // Mono to stereo (1 -> 2, 4)
  if (numberOfSourceChannels == 1 && (numberOfChannels == 2 || numberOfChannels == 4)) {
    AudioArray* sourceChannel = source.getChannelByType(ChannelMono);

    getChannelByType(ChannelLeft)->sum(sourceChannel);
    getChannelByType(ChannelRight)->sum(sourceChannel);
    return;
  }

  // Mono to 5.1 (1 -> 6)
  if (numberOfSourceChannels == 1 && numberOfChannels == 6) {
    AudioArray* sourceChannel = source.getChannel(0);

    getChannelByType(ChannelCenter)->sum(sourceChannel);
    return;
  }

  // Stereo 2 to stereo 4 or 5.1 (2 -> 4, 6)
  if (numberOfSourceChannels == 2 && (numberOfChannels == 4 || numberOfChannels == 6)) {

    getChannelByType(ChannelLeft)->sum(source.getChannelByType(ChannelLeft));
    getChannelByType(ChannelRight)->sum(source.getChannelByType(ChannelRight));
    return;
  }

  // Stereo 4 to 5.1 (4 -> 6)
  if (numberOfSourceChannels == 4 && numberOfChannels == 6) {
    getChannelByType(ChannelLeft)->sum(source.getChannelByType(ChannelLeft));
    getChannelByType(ChannelRight)->sum(source.getChannelByType(ChannelRight));
    getChannelByType(ChannelSurroundLeft)->sum(source.getChannelByType(ChannelSurroundLeft));
    getChannelByType(ChannelSurroundRight)->sum(source.getChannelByType(ChannelSurroundRight));
    return;
  }

  discreteSum(source);
};

void AudioBus::sumByDownMixing(const AudioBus &source) {
  int numberOfSourceChannels = source.getNumberOfChannels();
  int numberOfChannels = getNumberOfChannels();

  // Stereo to mono (2 -> 1): output += 0.5 * (input.left + input.right).
  if (numberOfSourceChannels == 2 && numberOfChannels == 1) {
    float* sourceLeft = source.getChannelByType(ChannelLeft)->getData();
    float* sourceRight = source.getChannelByType(ChannelRight)->getData();

    float* destinationData = getChannelByType(ChannelMono)->getData();

    VectorMath::multiplyByScalarThenAddToOutput(sourceLeft, 0.5f, destinationData, getSize());
    VectorMath::multiplyByScalarThenAddToOutput(sourceRight, 0.5f, destinationData, getSize());
    return;
  }

  // Stereo 4 to mono: output += 0.25 * (input.left + input.right + input.surroundLeft + input.surroundRight)
  if (numberOfSourceChannels == 4 && numberOfChannels == 1) {
    float* sourceLeft = source.getChannelByType(ChannelLeft)->getData();
    float* sourceRight = source.getChannelByType(ChannelRight)->getData();
    float* sourceSurroundLeft = source.getChannelByType(ChannelSurroundLeft)->getData();
    float* sourceSurroundRight = source.getChannelByType(ChannelSurroundRight)->getData();

    float* destinationData = getChannelByType(ChannelMono)->getData();

    VectorMath::multiplyByScalarThenAddToOutput(sourceLeft, 0.25f, destinationData, getSize());
    VectorMath::multiplyByScalarThenAddToOutput(sourceRight, 0.25f, destinationData, getSize());
    VectorMath::multiplyByScalarThenAddToOutput(sourceSurroundLeft, 0.25f, destinationData, getSize());
    VectorMath::multiplyByScalarThenAddToOutput(sourceSurroundRight, 0.25f, destinationData, getSize());
    return;
  }

  // 5.1 to stereo:
  // output.left += input.left + sqrt(1/2) * (input.center + input.surroundLeft)
  // output.right += input.right + sqrt(1/2) * (input.center + input.surroundRight)
  if (numberOfSourceChannels == 6 && numberOfChannels == 2) {
    float* sourceLeft = source.getChannelByType(ChannelLeft)->getData();
    float* sourceRight = source.getChannelByType(ChannelRight)->getData();
    float* sourceCenter = source.getChannelByType(ChannelCenter)->getData();
    float* sourceSurroundLeft = source.getChannelByType(ChannelSurroundLeft)->getData();
    float* sourceSurroundRight = source.getChannelByType(ChannelSurroundRight)->getData();

    float* destinationLeft = getChannelByType(ChannelLeft)->getData();
    float* destinationRight = getChannelByType(ChannelRight)->getData();

    VectorMath::add(sourceLeft, destinationLeft, destinationLeft, getSize());
    VectorMath::multiplyByScalarThenAddToOutput(sourceCenter, SQRT_HALF, destinationLeft, getSize());
    VectorMath::multiplyByScalarThenAddToOutput(sourceSurroundLeft, SQRT_HALF, destinationLeft, getSize());

    VectorMath::add(sourceRight, destinationRight, destinationRight, getSize());
    VectorMath::multiplyByScalarThenAddToOutput(sourceCenter, SQRT_HALF, destinationRight, getSize());
    VectorMath::multiplyByScalarThenAddToOutput(sourceSurroundRight, SQRT_HALF, destinationRight, getSize());
    return;
  }

  // 5.1 to stereo 4:
  // output.left += input.left + sqrt(1/2) * input.center
  // output.right += input.right + sqrt(1/2) * input.center
  // output.surroundLeft += input.surroundLeft
  // output.surroundRight += input.surroundRight
  if (numberOfSourceChannels == 6 && numberOfChannels == 4) {
    float* sourceLeft = source.getChannelByType(ChannelLeft)->getData();
    float* sourceRight = source.getChannelByType(ChannelRight)->getData();
    float* sourceCenter = source.getChannelByType(ChannelCenter)->getData();
    float* sourceSurroundLeft = source.getChannelByType(ChannelSurroundLeft)->getData();
    float* sourceSurroundRight = source.getChannelByType(ChannelSurroundRight)->getData();

    float* destinationLeft = getChannelByType(ChannelLeft)->getData();
    float* destinationRight = getChannelByType(ChannelRight)->getData();
    float* destinationSurroundLeft = getChannelByType(ChannelSurroundLeft)->getData();
    float* destinationSurroundRight = getChannelByType(ChannelSurroundRight)->getData();

    VectorMath::add(sourceLeft, destinationLeft, destinationLeft, getSize());
    VectorMath::multiplyByScalarThenAddToOutput(sourceCenter, SQRT_HALF, destinationLeft, getSize());

    VectorMath::add(sourceRight, destinationRight, destinationRight, getSize());
    VectorMath::multiplyByScalarThenAddToOutput(sourceCenter, SQRT_HALF, destinationRight, getSize());

    VectorMath::add(sourceSurroundLeft, destinationSurroundLeft, destinationSurroundLeft, getSize());
    VectorMath::add(sourceSurroundRight, destinationSurroundRight, destinationSurroundRight, getSize());
    return;
  }

  discreteSum(source);
};

} // namespace audioapi
