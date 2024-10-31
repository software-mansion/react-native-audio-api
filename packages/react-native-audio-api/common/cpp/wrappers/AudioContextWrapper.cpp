#include "AudioContextWrapper.h"

namespace audioapi {

AudioContextWrapper::AudioContextWrapper(
    const std::shared_ptr<AudioContext> &context)
    : BaseAudioContextWrapper(context) {}

void AudioContextWrapper::close() {
  getAudioContextFromBaseAudioContext()->close();
}

std::shared_ptr<AudioContext>
AudioContextWrapper::getAudioContextFromBaseAudioContext() {
  return std::static_pointer_cast<AudioContext>(context_);
}
} // namespace audioapi
