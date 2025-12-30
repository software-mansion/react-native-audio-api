#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/AudioNodeManager.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AudioNode::AudioNode(std::shared_ptr<BaseAudioContext> context)
    : context_(context),
      audioBus_(
          std::make_shared<AudioBus>(
              RENDER_QUANTUM_SIZE,
              channelCount_,
              context->getSampleRate())) {}

AudioNode::~AudioNode() {
  if (isInitialized_) {
    cleanup();
  }
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
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    context->getNodeManager()->addPendingNodeConnection(
        shared_from_this(), node, AudioNodeManager::ConnectionType::CONNECT);
  }
}

void AudioNode::connect(const std::shared_ptr<AudioParam> &param) {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    context->getNodeManager()->addPendingParamConnection(
        shared_from_this(), param, AudioNodeManager::ConnectionType::CONNECT);
  }
}

void AudioNode::disconnect() {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    context->getNodeManager()->addPendingNodeConnection(
        shared_from_this(), nullptr, AudioNodeManager::ConnectionType::DISCONNECT_ALL);
  }
}

void AudioNode::disconnect(const std::shared_ptr<AudioNode> &node) {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    context->getNodeManager()->addPendingNodeConnection(
        shared_from_this(), node, AudioNodeManager::ConnectionType::DISCONNECT);
  }
}

void AudioNode::disconnect(const std::shared_ptr<AudioParam> &param) {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    context->getNodeManager()->addPendingParamConnection(
        shared_from_this(), param, AudioNodeManager::ConnectionType::DISCONNECT);
  }
}

bool AudioNode::isEnabled() const {
  return isEnabled_;
}

bool AudioNode::requiresTailProcessing() const {
  return requiresTailProcessing_;
}

void AudioNode::enable() {
  if (isEnabled()) {
    return;
  }

  isEnabled_ = true;

  for (auto it = outputNodes_.begin(), end = outputNodes_.end(); it != end; ++it) {
    it->get()->onInputEnabled();
  }
}

void AudioNode::disable() {
  if (!isEnabled()) {
    return;
  }

  isEnabled_ = false;

  for (auto it = outputNodes_.begin(), end = outputNodes_.end(); it != end; ++it) {
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

std::shared_ptr<AudioBus> AudioNode::processAudio(
    const std::shared_ptr<AudioBus> &outputBus,
    int framesToProcess,
    bool checkIsAlreadyProcessed) {
  if (!isInitialized_) {
    return outputBus;
  }

  if (checkIsAlreadyProcessed && isAlreadyProcessed()) {
    return audioBus_;
  }

  // Process inputs and return the bus with the most channels.
  auto processingBus = processInputs(outputBus, framesToProcess, checkIsAlreadyProcessed);

  // Apply channel count mode.
  processingBus = applyChannelCountMode(processingBus);

  // Mix all input buses into the processing bus.
  mixInputsBuses(processingBus);

  assert(processingBus != nullptr);

  // Finally, process the node itself.
  return processNode(processingBus, framesToProcess);
}

bool AudioNode::isAlreadyProcessed() {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    std::size_t currentSampleFrame = context->getCurrentSampleFrame();

    // check if the node has already been processed for this rendering quantum
    if (currentSampleFrame == lastRenderedFrame_) {
      return true;
    }

    // Update the last rendered frame before processing node and its inputs.
    lastRenderedFrame_ = currentSampleFrame;

    return false;
  }

  // If context is invalid, consider it as already processed to avoid processing
  return true;
}

std::shared_ptr<AudioBus> AudioNode::processInputs(
    const std::shared_ptr<AudioBus> &outputBus,
    int framesToProcess,
    bool checkIsAlreadyProcessed) {
  auto processingBus = audioBus_;
  processingBus->zero();

  int maxNumberOfChannels = 0;
  for (auto it = inputNodes_.begin(), end = inputNodes_.end(); it != end; ++it) {
    auto inputNode = *it;
    assert(inputNode != nullptr);

    if (!inputNode->isEnabled()) {
      continue;
    }

    auto inputBus = inputNode->processAudio(outputBus, framesToProcess, checkIsAlreadyProcessed);
    inputBuses_.push_back(inputBus);

    if (maxNumberOfChannels < inputBus->getNumberOfChannels()) {
      maxNumberOfChannels = inputBus->getNumberOfChannels();
      processingBus = inputBus;
    }
  }

  return processingBus;
}

std::shared_ptr<AudioBus> AudioNode::applyChannelCountMode(
    const std::shared_ptr<AudioBus> &processingBus) {
  // If the channelCountMode is EXPLICIT, the node should output the number of
  // channels specified by the channelCount.
  if (channelCountMode_ == ChannelCountMode::EXPLICIT) {
    return audioBus_;
  }

  // If the channelCountMode is CLAMPED_MAX, the node should output the maximum
  // number of channels clamped to channelCount.
  if (channelCountMode_ == ChannelCountMode::CLAMPED_MAX &&
      processingBus->getNumberOfChannels() >= channelCount_) {
    return audioBus_;
  }

  return processingBus;
}

void AudioNode::mixInputsBuses(const std::shared_ptr<AudioBus> &processingBus) {
  assert(processingBus != nullptr);

  for (auto it = inputBuses_.begin(), end = inputBuses_.end(); it != end; ++it) {
    processingBus->sum(it->get(), channelInterpretation_);
  }

  inputBuses_.clear();
}

void AudioNode::connectNode(const std::shared_ptr<AudioNode> &node) {
  auto position = outputNodes_.find(node);

  if (position == outputNodes_.end()) {
    outputNodes_.insert(node);
    node->onInputConnected(this);
  }
}

void AudioNode::connectParam(const std::shared_ptr<AudioParam> &param) {
  auto position = outputParams_.find(param);

  if (position == outputParams_.end()) {
    outputParams_.insert(param);
    param->addInputNode(this);
  }
}

void AudioNode::disconnectNode(const std::shared_ptr<AudioNode> &node) {
  auto position = outputNodes_.find(node);

  if (position != outputNodes_.end()) {
    node->onInputDisconnected(this);
    outputNodes_.erase(node);
  }
}

void AudioNode::disconnectParam(const std::shared_ptr<AudioParam> &param) {
  auto position = outputParams_.find(param);

  if (position != outputParams_.end()) {
    param->removeInputNode(this);
    outputParams_.erase(param);
  }
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

  if (node->isEnabled()) {
    onInputDisabled();
  }

  auto position = inputNodes_.find(node);

  if (position != inputNodes_.end()) {
    inputNodes_.erase(position);
  }
}

void AudioNode::cleanup() {
  isInitialized_ = false;

  for (auto it = outputNodes_.begin(), end = outputNodes_.end(); it != end; ++it) {
    it->get()->onInputDisconnected(this);
  }

  outputNodes_.clear();
}

} // namespace audioapi
