#include "AudioBus.h"
#include "AudioNode.h"
#include "BaseAudioContext.h"

namespace audioapi {

AudioNode::AudioNode(BaseAudioContext *context) : context_(context) {
  audioBus_ = std::make_unique<AudioBus>(context->getSampleRate(), context->getBufferSizeInFrames(), channelCount_);
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

AudioBus* AudioNode::processAudio(AudioBus* outputBus, int framesToProcess) {
  std::size_t currentSampleFrame = context_->getCurrentSampleFrame();

  // check if the node has already been processed for this rendering quantum
  bool isAlreadyProcessed = currentSampleFrame == lastRenderedFrame_;

  // Node can't use output bus if:
  // - outputBus is not provided, which means that next node is doing a multi-node summing.
  // - it has more than one input, which means that it has to sum all inputs using internal bus.
  // - it has more than one output, so each output node can get the processed data without re-calculating the node.
  bool canUseOutputBus = outputBus != 0 && inputNodes_.size() == 1 && outputNodes_.size() == 1;

  if (isAlreadyProcessed) {
    // If it was already processed in the rendering quantum,return it.
    return audioBus_.get();
  }

  // Update the last rendered frame before processing node and its inputs.
  lastRenderedFrame_ = currentSampleFrame;

  AudioBus* processingBus = canUseOutputBus ? outputBus : audioBus_.get();

  if (!canUseOutputBus) {
    // Clear the bus before summing all connected nodes.
    processingBus->zero();
  }

  if (inputNodes_.empty()) {
    // If there are no connected inputs, process the node just to advance the audio params.
    // The node will output silence anyway.
    processNode(processingBus, framesToProcess);
    return processingBus;
  }

  // Process first connected node, it can be directly connected to the processingBus,
  // resulting in one less summing operation in most cases.
  AudioBus* firstNodeResultBus = inputNodes_.front()->processAudio(processingBus, framesToProcess);

  if (firstNodeResultBus != processingBus) {
    processingBus->sum(*firstNodeResultBus);
  }

  if (inputNodes_.size() > 1) {
    // If there are more connected nodes, sum them together.
    for (auto it = inputNodes_.begin() + 1; it != inputNodes_.end(); it += 1) {
      // Enforce the summing to be done using the internal bus.
      AudioBus* inputBus = it->get()->processAudio(0, framesToProcess);

      processingBus->sum(*inputBus);
    }
  }

  // Finally, process the node itself.
  processNode(processingBus, framesToProcess);

  return processingBus;
}

void AudioNode::cleanup() {
  outputNodes_.clear();
  inputNodes_.clear();
}

} // namespace audioapi
