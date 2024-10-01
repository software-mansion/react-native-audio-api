#include "AudioNode.h"

namespace audioapi {

int AudioNode::getNumberOfInputs() const {
  return numberOfInputs_;
}

int AudioNode::getNumberOfOutputs() const {
  return numberOfOutputs_;
}

int AudioNode::getChannelCount() const {
  return channelCount_;
}

std::string AudioNode::getChannelCountMode() const {
  return channelCountMode_;
}

std::string AudioNode::getChannelInterpretation() const {
  return channelInterpretation_;
}

void AudioNode::connect(const std::shared_ptr<AudioNode> &node) const {}

void AudioNode::disconnect(const std::shared_ptr<AudioNode> &node) const {}

} // namespace audioapi
