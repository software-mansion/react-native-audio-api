#ifndef ANDROID
#include "AudioBufferSourceNodeWrapper.h"

// TODO implement AudioBufferSourceNodeWrapper for iOS

namespace audioapi {

AudioBufferSourceNodeWrapper::AudioBufferSourceNodeWrapper(
    const std::shared_ptr<IOSAudioBufferSourceNode> &bufferSourceNode)
    : AudioScheduledSourceNodeWrapper(bufferSourceNode) {}

std::shared_ptr<IOSAudioBufferSourceNode>
AudioBufferSourceNodeWrapper::getAudioBufferSourceNodeFromAudioNode() {
  return std::static_pointer_cast<IOSAudioBufferSourceNode>(node_);
}

void AudioBufferSourceNodeWrapper::setLoop(bool loop) {
  auto bufferSource = getAudioBufferSourceNodeFromAudioNode();
  bufferSource->setLoop(loop);
}

bool AudioBufferSourceNodeWrapper::getLoop() {
  auto bufferSource = getAudioBufferSourceNodeFromAudioNode();
  return bufferSource->getLoop();
}

std::shared_ptr<AudioBufferWrapper> AudioBufferSourceNodeWrapper::getBuffer() {
  auto bufferSource = getAudioBufferSourceNodeFromAudioNode();
  auto buffer = bufferSource->getBuffer();
  return std::make_shared<AudioBufferWrapper>(buffer);
}

void AudioBufferSourceNodeWrapper::setBuffer(
    const std::shared_ptr<AudioBufferWrapper> &buffer) {
  auto bufferSource = getAudioBufferSourceNodeFromAudioNode();
  bufferSource->setBuffer(buffer->audioBuffer_);
}
} // namespace audioapi
#endif
