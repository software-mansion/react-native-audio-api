#include "AudioBus.h"
#include "AudioNode.h"
#include "BaseAudioContext.h"

namespace audioapi {

AudioNode::AudioNode(BaseAudioContext *context) : context_(context) {
  audioBus_ = std::make_unique<AudioBus>(context_, channelCount_);
}

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
  outputNodes_.push_back(node);
  node->inputNodes_.push_back(shared_from_this());
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


std::string AudioNode::toString(ChannelCountMode mode) {
  switch (mode) {
    case ChannelCountMode::MAX:
      return "max";
    case ChannelCountMode::CLAMPED_MAX:
      return "clamped-max";
    case ChannelCountMode::EXPLICIT:
      return "explicit";
    default:
      throw std::invalid_argument("Unknown channel count mode");
  }
}

std::string AudioNode::toString(ChannelInterpretation interpretation) {
  switch (interpretation) {
    case ChannelInterpretation::SPEAKERS:
      return "speakers";
    case ChannelInterpretation::DISCRETE:
      return "discrete";
    default:
      throw std::invalid_argument("Unknown channel interpretation");
  }
}

bool AudioNode::processAudio(AudioBus *outputBus, int framesToProcess) {
  // bool isPlaying = false;
  // for (auto &node : inputNodes_) {
  //   if (node->processAudio(buffer_.get(), framesToProcess)) {
  //     isPlaying = true;
  //   }
  // }

  // if (isPlaying) {
  //   outputBus->copyFrom(*buffer_.get(), 0, 0, framesToProcess);
  // }

  // return isPlaying;
}

void AudioNode::cleanup() {
  outputNodes_.clear();
  inputNodes_.clear();
}

} // namespace audioapi
