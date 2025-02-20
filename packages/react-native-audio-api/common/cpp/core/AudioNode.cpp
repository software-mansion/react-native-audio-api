#include <cassert>
#include <memory>

#include "AudioBus.h"
#include "AudioNode.h"
#include "AudioNodeManager.h"
#include "BaseAudioContext.h"

namespace audioapi {

AudioNode::AudioNode(BaseAudioContext *context) : context_(context) {
  audioBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE, channelCount_, context->getSampleRate());
}

AudioNode::~AudioNode() {
  isInitialized_ = false;
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

void AudioNode::disconnect() {
  for (auto &outputNode : outputNodes_) {
    disconnect(outputNode);
  }
}

void AudioNode::disconnect(const std::shared_ptr<AudioNode> &node) {
  context_->getNodeManager()->addPendingConnection(
      shared_from_this(), node, AudioNodeManager::ConnectionType::DISCONNECT);
}

bool AudioNode::isEnabled() const {
  return isEnabled_;
}

void AudioNode::enable() {
  isEnabled_ = true;

  for (auto &outputNode : outputNodes_) {
    outputNode->onInputEnabled();
  }
}

void AudioNode::disable() {
  isEnabled_ = false;

  for (auto &outputNode : outputNodes_) {
    outputNode->onInputDisabled();
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

  if (isAlreadyProcessed()) {
    return audioBus_.get();
  }

  // Process inputs and return the bus with the most channels.
  AudioBus* processingBus = processInputs(outputBus, framesToProcess);

  // Apply channel count mode.
  processingBus = applyChannelCountMode(processingBus);

  // Mix all input buses into the processing bus.
  mixInputsBuses(processingBus);

  assert(processingBus != nullptr);
  // Finally, process the node itself.
  processNode(processingBus, framesToProcess);

  return processingBus;
}

bool AudioNode::isAlreadyProcessed() {
    assert(context_ != nullptr);

    std::size_t currentSampleFrame = context_->getCurrentSampleFrame();

    // check if the node has already been processed for this rendering quantum
    if (currentSampleFrame == lastRenderedFrame_) {
        return true;
    }

    // Update the last rendered frame before processing node and its inputs.
    lastRenderedFrame_ = currentSampleFrame;

    return false;
}

AudioBus *AudioNode::processInputs(AudioBus *outputBus, int framesToProcess) {
  AudioBus* processingBus = audioBus_.get();
  processingBus->zero();

  int maxNumberOfChannels = 0;
  for (auto inputNode : inputNodes_) {
    assert(inputNode != nullptr);

    if (!inputNode->isEnabled()) {
      continue;
    }

    auto inputBus = inputNode->processAudio(outputBus, framesToProcess);
    inputBuses_.push_back(inputBus);

    if (maxNumberOfChannels < inputBus->getNumberOfChannels()) {
      maxNumberOfChannels = inputBus->getNumberOfChannels();
      processingBus = inputBus;
    }
  }

  return processingBus;
}

AudioBus *AudioNode::applyChannelCountMode(AudioBus *processingBus) {
  // If the channelCountMode is EXPLICIT, the node should output the number of channels specified by the channelCount.
  if (channelCountMode_ == ChannelCountMode::EXPLICIT) {
    return audioBus_.get();
  }

  // If the channelCountMode is CLAMPED_MAX, the node should output the maximum number of channels
  // clamped to channelCount.
  if (channelCountMode_ == ChannelCountMode::CLAMPED_MAX && processingBus->getNumberOfChannels() >= channelCount_) {
    return audioBus_.get();
  }

  return processingBus;
}

void AudioNode::mixInputsBuses(AudioBus *processingBus) {
  assert(processingBus != nullptr);

  for (auto inputBus : inputBuses_) {
    processingBus->sum(inputBus, channelInterpretation_);
  }

  inputBuses_.clear();
}

void AudioNode::connectNode(const std::shared_ptr<AudioNode> &node) {
  outputNodes_.insert(node);
  node->onInputConnected(this);
}

void AudioNode::disconnectNode(const std::shared_ptr<AudioNode> &node) {
  outputNodes_.erase(node);
  node->onInputDisconnected(this);
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
  if (!isInitialized_) {
    return;
  }

  inputNodes_.insert(node);

  if (node->isEnabled()) {
    onInputEnabled();
  }
}

void AudioNode::onInputDisconnected(AudioNode *node) {
  if (!isInitialized_) {
    return;
  }

  inputNodes_.erase(node);

  if (isEnabled()) {
    node->onInputDisabled();
  }
}

void AudioNode::cleanup() {
  isInitialized_ = false;

  for (const auto &outputNode : outputNodes_) {
    outputNode->onInputDisconnected(this);
  }

  for (const auto &inputNode : inputNodes_) {
    inputNode->disconnectNode(shared_from_this());
  }

  outputNodes_.clear();
  inputNodes_.clear();
}

} // namespace audioapi
