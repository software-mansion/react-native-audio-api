#include <audioapi/encoding/ChannelRemixer.h>

#include <cstring>
#include <memory>

namespace audioapi {

ChannelRemixer::ChannelRemixer(
    int inputChannelCount,
    int outputChannelCount,
    size_t maxFrames,
    float sampleRate)
    : inputChannelCount_(inputChannelCount), outputChannelCount_(outputChannelCount) {
  if (inputChannelCount != outputChannelCount) {
    inputPlanar_ = std::make_unique<AudioBuffer>(maxFrames, inputChannelCount, sampleRate);
    remixedPlanar_ = std::make_unique<AudioBuffer>(maxFrames, outputChannelCount, sampleRate);
  }
}

ChannelRemixer::ChannelPointers ChannelRemixer::remix(const float *const *channels, size_t frames) {
  ChannelPointers remixed{};
  if (remixedPlanar_ == nullptr) {
    for (int channel = 0; channel < outputChannelCount_; ++channel) {
      remixed[channel] = channels[channel];
    }
    return remixed;
  }

  for (int channel = 0; channel < inputChannelCount_; ++channel) {
    std::memcpy(
        inputPlanar_->getChannel(channel)->begin(), channels[channel], frames * sizeof(float));
  }
  remixedPlanar_->copy(*inputPlanar_, 0, 0, frames);
  for (int channel = 0; channel < outputChannelCount_; ++channel) {
    remixed[channel] = remixedPlanar_->getChannel(channel)->begin();
  }
  return remixed;
}

} // namespace audioapi
