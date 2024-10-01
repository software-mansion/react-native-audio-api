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

void AudioNode::connect(const std::shared_ptr<AudioNode> &node) {
  if (numberOfOutputs_ > outputNodes_.size() && node->getNumberOfInputs() > node->inputNodes_.size()) {
    outputNodes_.push_back(node);
    node->inputNodes_.push_back(shared_from_this());
  }
}

void AudioNode::disconnect(const std::shared_ptr<AudioNode> &node){
    outputNodes_.erase(std::remove(outputNodes_.begin(), outputNodes_.end(), node), outputNodes_.end());
    if (auto sharedThis = shared_from_this()) {
        node->inputNodes_.erase(std::remove(node->inputNodes_.begin(), node->inputNodes_.end(), sharedThis), node->inputNodes_.end());
    }
}

void AudioNode::process(AudioStream *oboeStream,
                        void *audioData,
                        int32_t numFrames, int channelCount) {
    std::for_each(outputNodes_.begin(), outputNodes_.end(), [&oboeStream, &audioData, &numFrames, &channelCount](const std::shared_ptr<AudioNode>& node) {
        node->process(oboeStream, audioData, numFrames, channelCount);
    });
}

} // namespace audioapi
