#include "AudioNode.h"
#include "AudioContext.h"

namespace audioapi {

AudioNode::AudioNode(AudioContext *context) : context_(context) {}

AudioNode::~AudioNode() {
  cleanup();
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

void AudioNode::cleanup() {
  outputNodes_.clear();
  inputNodes_.clear();
}

bool AudioNode::processAudio(float *audioData, int32_t numFrames) {
    bool isPlaying = false;
    for (auto &node: inputNodes_) {
        if (node->processAudio(audioData, numFrames)) {
            isPlaying = true;
        }
    }

    return isPlaying;
}

} // namespace audioapi
