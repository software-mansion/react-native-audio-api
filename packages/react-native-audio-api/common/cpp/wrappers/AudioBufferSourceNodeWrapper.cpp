#include "AudioBufferSourceNodeWrapper.h"

namespace audioapi {

AudioBufferSourceNodeWrapper::AudioBufferSourceNodeWrapper(
    const std::shared_ptr<AudioBufferSourceNode> &audioBufferSourceNode)
    : AudioScheduledSourceNodeWrapper(audioBufferSourceNode) {}

std::shared_ptr<AudioBufferSourceNode>
AudioBufferSourceNodeWrapper::getAudioBufferSourceNodeFromAudioNode() {
  return std::static_pointer_cast<AudioBufferSourceNode>(node_);
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

  if (!buffer) {
    return {nullptr};
  }

  return std::make_shared<AudioBufferWrapper>(buffer);
}

void AudioBufferSourceNodeWrapper::setBuffer(
    const std::shared_ptr<AudioBufferWrapper> &buffer) {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  if (!buffer) {
    audioBufferSourceNode->setBuffer(std::shared_ptr<AudioBuffer>(nullptr));
    return;
  }

  audioBufferSourceNode->setBuffer(buffer->audioBuffer_);
}
} // namespace audioapi
