#include "AudioNode.h"
#include "AudioContext.h"

namespace audioapi {

AudioNode::AudioNode(AudioContext *context) : context_(context) {
    inputBuffer_ = std::vector<float>(CHANNEL_COUNT * context_->getBufferSize());
    outputBuffer_ = std::vector<float>(CHANNEL_COUNT * context_->getBufferSize());
}

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
  return AudioNode::toString(channelCountMode_);
}

std::string AudioNode::getChannelInterpretation() const {
  return AudioNode::toString(channelInterpretation_);
}

void AudioNode::connect(const std::shared_ptr<AudioNode> &node) {
  if (numberOfOutputs_ > outputNodes_.size() &&
      node->getNumberOfInputs() > node->inputNodes_.size()) {
    outputNodes_.push_back(node);
    node->inputNodes_.push_back(shared_from_this());
  }
}

void AudioNode::disconnect(const std::shared_ptr<AudioNode> &node) {
  outputNodes_.erase(
      std::remove(outputNodes_.begin(), outputNodes_.end(), node),
      outputNodes_.end());
  if (auto sharedThis = shared_from_this()) {
    node->inputNodes_.erase(
        std::remove(
            node->inputNodes_.begin(), node->inputNodes_.end(), sharedThis),
        node->inputNodes_.end());
  }
}

void AudioNode::processAudio() {
    for (const auto & inputNode : inputNodes_) {
        inputNode->processAudio();
    }

    mixInputBuffers();
}

void AudioNode::cleanup() {
  std::for_each(
      outputNodes_.begin(),
      outputNodes_.end(),
      [](const std::shared_ptr<AudioNode> &node) { node->cleanup(); });

  outputNodes_.clear();
  inputNodes_.clear();
}

void AudioNode::mixInputBuffers() {
    float value;
    for (int i = 0; i < inputBuffer_.size(); i++) {
        value = 0;
        for (const auto & inputNode : inputNodes_) {
            value += inputNode->outputBuffer_[i];
        }
        inputBuffer_[i] = value;
    }
}

} // namespace audioapi
