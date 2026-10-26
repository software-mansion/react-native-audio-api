#include <audioapi/core/sources/SharedAudioBuffer.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

namespace audioapi {

SharedAudioBuffer::SharedAudioBuffer(std::shared_ptr<AudioBuffer> samples)
    : samples_(std::move(samples)),
      channelSharedWithNode_(samples_->getNumberOfChannels(), false),
      channelExposed_(samples_->getNumberOfChannels(), false) {}

std::shared_ptr<AudioArrayBuffer> SharedAudioBuffer::exposeChannel(size_t channel) {
  makeChannelWritable(channel);
  channelExposed_[channel] = true;
  return samples_->getSharedChannel(channel);
}

void SharedAudioBuffer::copyFromChannel(
    float *destination,
    size_t destinationLength,
    size_t channel,
    double startInChannel) const {
  auto channelSize = samples_->getSize();
  if (startInChannel < 0 || startInChannel >= static_cast<double>(channelSize)) {
    return;
  }

  auto start = static_cast<size_t>(startInChannel);
  auto framesToCopy = std::min(destinationLength, channelSize - start);
  samples_->getChannel(channel)->copyTo(destination, start, 0, framesToCopy);
}

void SharedAudioBuffer::copyToChannel(
    const float *source,
    size_t sourceLength,
    size_t channel,
    double startInChannel) {
  auto channelSize = samples_->getSize();
  if (startInChannel < 0 || startInChannel >= static_cast<double>(channelSize)) {
    return;
  }

  makeChannelWritable(channel);
  auto start = static_cast<size_t>(startInChannel);
  auto framesToCopy = std::min(sourceLength, channelSize - start);
  samples_->getChannel(channel)->copy(source, 0, start, framesToCopy);
}

std::shared_ptr<AudioBuffer> SharedAudioBuffer::acquireContent() {
  for (size_t channel = 0; channel < channelExposed_.size(); ++channel) {
    if (channelExposed_[channel]) {
      replaceChannelStorage(channel);
      channelExposed_[channel] = false;
    }
  }

  channelSharedWithNode_.assign(samples_->getNumberOfChannels(), true);
  return samples_->shareChannels();
}

void SharedAudioBuffer::makeChannelWritable(size_t channel) {
  if (channelSharedWithNode_[channel]) {
    replaceChannelStorage(channel);
  }
}

void SharedAudioBuffer::replaceChannelStorage(size_t channel) {
  samples_->detachSharedChannel(channel);
  channelSharedWithNode_[channel] = false;
}

} // namespace audioapi
