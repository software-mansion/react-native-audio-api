#ifdef ANDROID
#include "AudioBufferSourceNodeWrapper.h"

namespace audioapi {

std::shared_ptr<AudioBufferSourceNode>
AudioBufferSourceNodeWrapper::getAudioBufferSourceNodeFromAudioNode() {
  return std::static_pointer_cast<AudioBufferSourceNode>(node_);
}

bool AudioBufferSourceNodeWrapper::getLoop() {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  // return audioBufferSourceNode->getLoop();
  return false;
}

void AudioBufferSourceNodeWrapper::setLoop(bool loop) {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  // audioBufferSourceNode->setLoop(loop);
}

std::shared_ptr<AudioBufferWrapper> AudioBufferSourceNodeWrapper::getBuffer() {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  // auto buffer = audioBufferSourceNode->getBuffer();
  // return std::make_shared<AudioBufferWrapper>(buffer);
  return nullptr;
}

void AudioBufferSourceNodeWrapper::setBuffer(
    const std::shared_ptr<AudioBufferWrapper> &buffer) {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  // audioBufferSourceNode->setBuffer(buffer->audioBuffer_);
}
} // namespace audioapi
#endif
