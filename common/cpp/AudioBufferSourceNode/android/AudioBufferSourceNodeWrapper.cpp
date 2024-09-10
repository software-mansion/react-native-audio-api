#ifdef ANDROID
#include "AudioBufferSourceNodeWrapper.h"

namespace audioapi {

AudioBufferSourceNode *
AudioBufferSourceNodeWrapper::getAudioBufferSourceNodeFromAudioNode() {
  return static_cast<AudioBufferSourceNode *>(node_);
}

bool AudioBufferSourceNodeWrapper::getLoop() {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  return audioBufferSourceNode->getLoop();
}

void AudioBufferSourceNodeWrapper::setLoop(bool loop) {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  audioBufferSourceNode->setLoop(loop);
}

std::shared_ptr<AudioBufferWrapper> AudioBufferSourceNodeWrapper::getBuffer() {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  auto buffer = audioBufferSourceNode->getBuffer();
  return std::make_shared<AudioBufferWrapper>(buffer);
}

void AudioBufferSourceNodeWrapper::setBuffer(
    const std::shared_ptr<AudioBufferWrapper> &buffer) {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  audioBufferSourceNode->setBuffer(buffer->audioBuffer_);
}
} // namespace audioapi
#endif
