#include "AudioNodeWrapper.h"

namespace audioapi {

AudioNodeWrapper::AudioNodeWrapper(const std::shared_ptr<AudioNode> &node)
    : node_(node) {}

int AudioNodeWrapper::getNumberOfInputs() const {
  return node_->getNumberOfInputs();
}

int AudioNodeWrapper::getNumberOfOutputs() const {
  return node_->getNumberOfOutputs();
}

int AudioNodeWrapper::getChannelCount() const {
  return node_->getChannelCount();
}

std::string AudioNodeWrapper::getChannelCountMode() const {
  return node_->getChannelCountMode();
}

std::string AudioNodeWrapper::getChannelInterpretation() const {
  return node_->getChannelInterpretation();
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
