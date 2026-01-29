#include <audioapi/HostObjects/utils/NodeOptions.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace audioapi {

AudioBuffer::AudioBuffer(const AudioBufferOptions &options)
    : bus_(std::make_shared<AudioBus>(options.length, options.numberOfChannels, options.sampleRate)) {}

AudioBuffer::AudioBuffer(std::shared_ptr<AudioBus> bus) {
  bus_ = std::move(bus);
}

AudioBuffer::AudioBuffer(const audioapi::AudioBuffer &other): bus_{std::make_shared<AudioBus>(*other.bus_)} {}

AudioBuffer &AudioBuffer::operator=(const audioapi::AudioBuffer &other) {
  if (this != &other) {
    bus_ = std::make_shared<AudioBus>(*other.bus_);
  }

  return *this;
}

AudioBuffer::AudioBuffer(audioapi::AudioBuffer &&other) noexcept: bus_{std::move(other.bus_)} {}

AudioBuffer &AudioBuffer::operator=(audioapi::AudioBuffer &&other) noexcept {
  if (this != &other) {
    bus_ = std::move(other.bus_);
  }

  return *this;
}

size_t AudioBuffer::getLength() const {
  return bus_->getSize();
}

int AudioBuffer::getNumberOfChannels() const {
  return bus_->getNumberOfChannels();
}

float AudioBuffer::getSampleRate() const {
  return bus_->getSampleRate();
}

double AudioBuffer::getDuration() const {
  return static_cast<double>(getLength()) / getSampleRate();
}

float *AudioBuffer::getChannelData(int channel) const {
  return bus_->getChannel(channel)->getData();
}

void AudioBuffer::copyFromChannel(
    float *destination,
    size_t destinationLength,
    int channelNumber,
    size_t startInChannel) const {
  memcpy(
      destination,
      bus_->getChannel(channelNumber)->getData() + startInChannel,
      std::min(destinationLength, getLength() - startInChannel) * sizeof(float));
}

void AudioBuffer::copyToChannel(
    const float *source,
    size_t sourceLength,
    int channelNumber,
    size_t startInChannel) {
  memcpy(
      bus_->getChannel(channelNumber)->getData() + startInChannel,
      source,
      std::min(sourceLength, getLength() - startInChannel) * sizeof(float));
}

} // namespace audioapi
