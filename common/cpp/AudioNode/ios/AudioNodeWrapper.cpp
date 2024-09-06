#ifndef ANDROID
#include "AudioNodeWrapper.h"

namespace audioapi {

AudioNodeWrapper::AudioNodeWrapper(std::shared_ptr<IOSAudioNode> node) {
    node_ = node;
}

int AudioNodeWrapper::getNumberOfInputs() const {
  return node_->getNumberOfInputs();
}

int AudioNodeWrapper::getNumberOfOutputs() const {
  return node_->getNumberOfOutputs();
}

int AudioNodeWrapper::getChannelCount() const {
  // TODO: implement
  return 0;
}

std::string AudioNodeWrapper::getChannelCountMode() const {
  // TODO: implement
  return "";
}

std::string AudioNodeWrapper::getChannelInterpretation() const {
  // TODO: implement
  return "";
}

void AudioNodeWrapper::connect(
    const std::shared_ptr<AudioNodeWrapper> &node) const {
  node_->connect(node->node_);
}

void AudioNodeWrapper::disconnect(
    const std::shared_ptr<AudioNodeWrapper> &node) const {
  node_->disconnect(node->node_);
}

} // namespace audioapi
#endif
