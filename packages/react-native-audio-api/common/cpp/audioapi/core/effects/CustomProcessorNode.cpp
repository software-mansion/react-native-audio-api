#include <audioapi/core/effects/CustomProcessorNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

#include <iostream>
#include <algorithm>
#include <cstring>

namespace audioapi {

// Static registries and active node tracking
std::map<std::string, std::function<std::shared_ptr<CustomAudioProcessor>()>> CustomProcessorNode::s_processorFactoriesByIdentifier;
std::unordered_map<std::string, CustomProcessorNode::GenericControlHandler> CustomProcessorNode::s_controlHandlersByIdentifier;
std::unordered_map<std::string, std::vector<CustomProcessorNode*>> CustomProcessorNode::s_activeNodes;

// Constructor: initializes processor, tracking, and preallocated buffers
CustomProcessorNode::CustomProcessorNode(BaseAudioContext* context, const std::string& identifier)
    : AudioNode(context), processorMode_(ProcessorMode::ProcessInPlace) {
  customProcessorParam_ = std::make_shared<AudioParam>(
      1.0f, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);
  isInitialized_ = true;

  auto it = s_processorFactoriesByIdentifier.find(identifier);
  processor_ = (it != s_processorFactoriesByIdentifier.end()) ? it->second() : nullptr;

  s_activeNodes[identifier].push_back(this);
}

// Destructor: cleans up tracking for this instance
CustomProcessorNode::~CustomProcessorNode() {
  for (auto& pair : s_activeNodes) {
    auto& vec = pair.second;
    vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
  }
}

// Gets the modifiable parameter
std::shared_ptr<AudioParam> CustomProcessorNode::getCustomProcessorParam() const {
  return customProcessorParam_;
}

// Returns the current processing mode
CustomProcessorNode::ProcessorMode CustomProcessorNode::getProcessorMode() const {
  return processorMode_;
}

// Updates the processing mode
void CustomProcessorNode::setProcessorMode(ProcessorMode mode) {
  processorMode_ = mode;
}

// Registers a factory for dynamic processor instantiation
void CustomProcessorNode::registerProcessorFactory(
    const std::string& identifier,
    std::function<std::shared_ptr<CustomAudioProcessor>()> factory) {
  s_processorFactoriesByIdentifier[identifier] = std::move(factory);
  notifyProcessorChanged(identifier);
}

// Removes a processor factory from registry
void CustomProcessorNode::unregisterProcessorFactory(const std::string& identifier) {
  s_processorFactoriesByIdentifier.erase(identifier);
  notifyProcessorChanged(identifier);
}

// Updates processor instances for all nodes using given identifier
void CustomProcessorNode::notifyProcessorChanged(const std::string& identifier) {
  auto it = s_activeNodes.find(identifier);
  if (it != s_activeNodes.end()) {
    for (CustomProcessorNode* node : it->second) {
      auto f = s_processorFactoriesByIdentifier.find(identifier);
      node->processor_ = (f != s_processorFactoriesByIdentifier.end()) ? f->second() : nullptr;
    }
  }
}

// Registers control handler for runtime automation
void CustomProcessorNode::registerControlHandler(
    const std::string& identifier,
    GenericControlHandler handler) {
  s_controlHandlersByIdentifier[identifier] = std::move(handler);
}

// Removes a control handler
void CustomProcessorNode::unregisterControlHandler(const std::string& identifier) {
  s_controlHandlersByIdentifier.erase(identifier);
}

// Core processing method routed by selected mode
void CustomProcessorNode::processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) {
  if (!processor_) return;

  int numChannels = processingBus->getNumberOfChannels();
  if (preallocatedOutputBuffers_.size() != static_cast<size_t>(numChannels)) {
    preallocatedOutputBuffers_.resize(numChannels);
  }

  for (int ch = 0; ch < numChannels; ++ch) {
    if (preallocatedOutputBuffers_[ch].size() != static_cast<size_t>(framesToProcess)) {
      preallocatedOutputBuffers_[ch].resize(framesToProcess);
    }
  }

  switch (processorMode_) {
    case ProcessorMode::ProcessThrough:
      processThrough(processingBus, framesToProcess);
      break;
    case ProcessorMode::ProcessInPlace:
    default:
      processInPlace(processingBus, framesToProcess);
      break;
  }
}

// Executes in-place processing on the shared audio buffer
void CustomProcessorNode::processInPlace(const std::shared_ptr<AudioBus>& bus, int frames) {
  if (!processor_) return;

  int numChannels = bus->getNumberOfChannels();
  std::vector<float*> channelData(numChannels);
  for (int ch = 0; ch < numChannels; ++ch) {
    channelData[ch] = bus->getChannel(ch)->getData();
  }

  processor_->processInPlace(channelData.data(), numChannels, frames);
}

// Executes processing using separate input and output buffers with preallocation
void CustomProcessorNode::processThrough(const std::shared_ptr<AudioBus>& bus, int frames) {
  if (!processor_) return;

  int numChannels = bus->getNumberOfChannels();
  std::vector<float*> input(numChannels);
  std::vector<float*> output(numChannels);

  for (int ch = 0; ch < numChannels; ++ch) {
    input[ch] = bus->getChannel(ch)->getData();
    output[ch] = preallocatedOutputBuffers_[ch].data();
  }

  processor_->processThrough(input.data(), output.data(), numChannels, frames);

  for (int ch = 0; ch < numChannels; ++ch) {
    std::memcpy(bus->getChannel(ch)->getData(), output[ch], sizeof(float) * frames);
  }
}

} // namespace audioapi
