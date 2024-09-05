#ifndef ANDROID
#include "AudioBufferSourceNodeWrapper.h"

// TODO implement AudioBufferSourceNodeWrapper for iOS

namespace audioapi {
void AudioBufferSourceNodeWrapper::start(double time) {
  return;
}

void AudioBufferSourceNodeWrapper::stop(double time) {
  return;
}

void AudioBufferSourceNodeWrapper::setLoop(bool loop) {
  return;
}

bool AudioBufferSourceNodeWrapper::getLoop() {
  return true;
}

std::shared_ptr<AudioBufferWrapper> AudioBufferSourceNodeWrapper::getBuffer() {
  return std::make_shared<AudioBufferWrapper>(nullptr);
}
} // namespace audioapi
#endif
