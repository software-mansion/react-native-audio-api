#include "AudioParamWrapper.h"
#include "AudioBufferSourceNodeWrapper.h"

namespace audioapi {

AudioBufferSourceNodeWrapper::AudioBufferSourceNodeWrapper(
    const std::shared_ptr<AudioBufferSourceNode> &audioBufferSourceNode)
    : AudioScheduledSourceNodeWrapper(audioBufferSourceNode) {
      auto detuneParam = audioBufferSourceNode->getDetuneParam();
      detuneParam_ = std::make_shared<AudioParamWrapper>(detuneParam);

      auto playbackRateParam = audioBufferSourceNode->getPlaybackRateParam();
      playbackRateParam_ = std::make_shared<AudioParamWrapper>(playbackRateParam);
}

std::shared_ptr<AudioBufferSourceNode>
AudioBufferSourceNodeWrapper::getAudioBufferSourceNodeFromAudioNode() {
  return std::static_pointer_cast<AudioBufferSourceNode>(node_);
}

bool AudioBufferSourceNodeWrapper::getLoop() {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  return audioBufferSourceNode->getLoop();
}

double AudioBufferSourceNodeWrapper::getLoopStart() {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  return audioBufferSourceNode->getLoopStart();
}

double AudioBufferSourceNodeWrapper::getLoopEnd() {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  return audioBufferSourceNode->getLoopEnd();
}

std::shared_ptr<AudioParamWrapper> AudioBufferSourceNodeWrapper::getDetuneParam() {
  return detuneParam_;
}

std::shared_ptr<AudioParamWrapper> AudioBufferSourceNodeWrapper::getPlaybackRateParam() {
  return playbackRateParam_;
}

std::shared_ptr<AudioBufferWrapper> AudioBufferSourceNodeWrapper::getBuffer() {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  auto buffer = audioBufferSourceNode->getBuffer();

  if (!buffer) {
    return {nullptr};
  }

  return std::make_shared<AudioBufferWrapper>(buffer);
}

void AudioBufferSourceNodeWrapper::setLoop(bool loop) {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  audioBufferSourceNode->setLoop(loop);
}

void AudioBufferSourceNodeWrapper::setLoopStart(double loopStart) {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  audioBufferSourceNode->setLoopStart(loopStart);
}

void AudioBufferSourceNodeWrapper::setLoopEnd(double loopEnd) {
  auto audioBufferSourceNode = getAudioBufferSourceNodeFromAudioNode();
  audioBufferSourceNode->setLoopEnd(loopEnd);
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
