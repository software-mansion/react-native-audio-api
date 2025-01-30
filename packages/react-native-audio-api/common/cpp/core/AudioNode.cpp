#include <memory>

#include "AudioBus.h"
#include "AudioNode.h"
#include "AudioNodeManager.h"
#include "BaseAudioContext.h"

namespace audioapi {

AudioNode::AudioNode(BaseAudioContext *context) : context_(context) {
  audioBus_ = std::make_shared<AudioBus>(
      context->getSampleRate(), RENDER_QUANTUM_SIZE, channelCount_);
}

AudioNode::~AudioNode() {
  isInitialized_ = false;
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
  context_->getNodeManager()->addPendingConnection(
      shared_from_this(), node, AudioNodeManager::ConnectionType::CONNECT);
}

void AudioNode::connectNode(const std::shared_ptr<AudioNode> &node) {
  outputNodes_.push_back(node);
  node->onInputConnected(this);
}

void AudioNode::disconnect() {
  for (auto &outputNode : outputNodes_) {
    disconnectNode(outputNode);
  }
}

void AudioNode::disconnect(const std::shared_ptr<AudioNode> &node) {
  context_->getNodeManager()->addPendingConnection(
      shared_from_this(), node, AudioNodeManager::ConnectionType::DISCONNECT);
}

void AudioNode::disconnectNode(const std::shared_ptr<AudioNode> &node) {
  node->onInputDisconnected(this);

  auto position = std::find(outputNodes_.begin(), outputNodes_.end(), node);

  if (position != outputNodes_.end()) {
    outputNodes_.erase(position);
  }
}

bool AudioNode::isEnabled() const {
  return isEnabled_;
}

void AudioNode::enable() {
  isEnabled_ = true;

  for (auto it = outputNodes_.begin(); it != outputNodes_.end(); ++it) {
    it->get()->onInputEnabled();
  }
}

void AudioNode::disable() {
  isEnabled_ = false;

  for (auto it = outputNodes_.begin(); it != outputNodes_.end(); ++it) {
    it->get()->onInputDisabled();
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

AudioBus *AudioNode::processAudio(AudioBus *outputBus, int framesToProcess) {
  if (!isInitialized_) {
    return outputBus;
  }

  std::size_t currentSampleFrame = context_->getCurrentSampleFrame();

  // check if the node has already been processed for this rendering quantum
  bool isAlreadyProcessed = currentSampleFrame == lastRenderedFrame_;

  // Node can't use output bus if:
  // - outputBus is not provided, which means that next node is doing a
  // multi-node summing.
  // - it has more than one input, which means that it has to sum all inputs
  // using internal bus.
  // - it has more than one output, so each output node can get the processed
  // data without re-calculating the node.
  bool canUseOutputBus =
      outputBus != nullptr && inputNodes_.size() < 2 && outputNodes_.size() < 2;

  if (isAlreadyProcessed) {
    // If it was already processed in the rendering quantum, return it.
    return audioBus_.get();
  }

  // Update the last rendered frame before processing node and its inputs.
  lastRenderedFrame_ = currentSampleFrame;

  AudioBus *processingBus = canUseOutputBus ? outputBus : audioBus_.get();

  if (!canUseOutputBus) {
    // Clear the bus before summing all connected nodes.
    processingBus->zero();
  }

  if (inputNodes_.empty()) {
    // If there are no connected inputs, if processing node is the source node,
    // it will fill processing bus with the audio data, otherwise it will return
    // silence.
    processNode(processingBus, framesToProcess);
    return processingBus;
  }

  for (auto it = inputNodes_.begin(); it != inputNodes_.end(); ++it) {
    if (!(*it)->isEnabled()) {
      continue;
    }

    // Process first connected node, it can be directly connected to the
    // processingBus, resulting in one less summing operation.
    if (it == inputNodes_.begin()) {
      AudioBus *inputBus = (*it)->processAudio(processingBus, framesToProcess);

      if (inputBus != processingBus) {
        processingBus->sum(inputBus);
      }
    } else {
      // Enforce the summing to be done using the internal bus.
      AudioBus *inputBus = (*it)->processAudio(0, framesToProcess);
      if (inputBus) {
        processingBus->sum(inputBus);
      }
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

void AudioNode::onInputEnabled() {
  numberOfEnabledInputNodes_ += 1;

  if (!isEnabled()) {
    enable();
  }
}

void AudioNode::onInputDisabled() {
  numberOfEnabledInputNodes_ -= 1;

  if (isEnabled() && numberOfEnabledInputNodes_ == 0) {
    disable();
  }
}

void AudioNode::onInputConnected(AudioNode *node) {
  inputNodes_.push_back(node);

  if (node->isEnabled()) {
    onInputEnabled();
  }
}

void AudioNode::onInputDisconnected(AudioNode *node) {
  auto position = std::find(inputNodes_.begin(), inputNodes_.end(), node);

  if (position != inputNodes_.end()) {
    inputNodes_.erase(position);
  }

  if (!inputNodes_.empty()) {
    return;
  }

  if (isEnabled()) {
    node->onInputDisabled();
  }

  for (const auto &outputNode : outputNodes_) {
    disconnectNode(outputNode);
  }
}

} // namespace audioapi
