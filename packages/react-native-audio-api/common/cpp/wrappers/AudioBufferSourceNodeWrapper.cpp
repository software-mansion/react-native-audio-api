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
    try {
        auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
        auto buffer = audioBufferSourceNode->getBuffer();
        return std::make_shared<AudioBufferWrapper>(buffer);
    } catch (const std::runtime_error &e) {
        throw std::runtime_error("Buffer is not set");
    }
}

void AudioBufferSourceNodeWrapper::setBuffer(
    const std::shared_ptr<AudioBufferWrapper> &buffer) {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  audioBufferSourceNode->setBuffer(buffer->audioBuffer_);
}

void AudioBufferSourceNodeWrapper::resetBuffer() {
    auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
    audioBufferSourceNode->resetBuffer();
}
} // namespace audioapi
