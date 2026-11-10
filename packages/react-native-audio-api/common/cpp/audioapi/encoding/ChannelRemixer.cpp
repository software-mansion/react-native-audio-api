#include <audioapi/encoding/ChannelRemixer.h>

#include <memory>

namespace audioapi {

ChannelRemixer::ChannelRemixer(
    int inputChannelCount,
    int outputChannelCount,
    size_t maxFrames,
    float sampleRate)
    : outputChannelCount_(outputChannelCount) {
  if (inputChannelCount != outputChannelCount) {
    remixed_ = std::make_unique<AudioBuffer>(maxFrames, outputChannelCount, sampleRate);
  }
}

const AudioBuffer &ChannelRemixer::remix(const AudioBuffer &input, size_t frames) {
  // input equals output, no remixing needed, just return the input channels
  if (remixed_ == nullptr) {
    return input;
  }
  remixed_->copy(input, 0, 0, frames);
  return *remixed_;
}

} // namespace audioapi
