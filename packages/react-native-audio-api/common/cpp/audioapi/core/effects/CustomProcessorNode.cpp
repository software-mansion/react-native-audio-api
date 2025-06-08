#include <audioapi/core/effects/CustomProcessorNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

#include <iostream>
#include <algorithm>
#include <cstring>

namespace audioapi {

// Static registry for processor factories by identifier
std::map<std::string, std::function<std::shared_ptr<CustomAudioProcessor>()>> CustomProcessorNode::s_processorFactoriesByIdentifier;

// Static registry for control handlers by identifier
std::unordered_map<std::string, CustomProcessorNode::GenericControlHandler> CustomProcessorNode::s_controlHandlersByIdentifier;

// Tracks all active CustomProcessorNode instances
std::vector<CustomProcessorNode*> CustomProcessorNode::activeNodes;

// Constructor: initializes the node and registers it in the active list
CustomProcessorNode::CustomProcessorNode(BaseAudioContext* context)
    : AudioNode(context), processorMode_("processInPlace") {
  customProcessorParam_ = std::make_shared<AudioParam>(
      1.0f, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);
  isInitialized_ = true;
  processor_ = nullptr;
  identifier_ = "";
  activeNodes.push_back(this);
}

// Destructor: removes the node from the active list
CustomProcessorNode::~CustomProcessorNode() {
  activeNodes.erase(std::remove(activeNodes.begin(), activeNodes.end(), this), activeNodes.end());
}

// Getter for the custom AudioParam (optional)
std::shared_ptr<AudioParam> CustomProcessorNode::getCustomProcessorParam() const {
  return customProcessorParam_;
}

// Returns the current identifier assigned to this node
std::string CustomProcessorNode::getIdentifier() const {
  return identifier_;
}

// Sets the node's identifier and binds the matching processor instance, if available
void CustomProcessorNode::setIdentifier(const std::string& identifier) {
  if (identifier_ != identifier || !processor_) {
    identifier_ = identifier;
    auto it = s_processorFactoriesByIdentifier.find(identifier_);
    processor_ = (it != s_processorFactoriesByIdentifier.end()) ? it->second() : nullptr;
  }
}

// Returns the current processor mode
std::string CustomProcessorNode::getProcessorMode() const {
  return processorMode_;
}

// Sets the processor mode, falling back to "processInPlace" on invalid input
void CustomProcessorNode::setProcessorMode(const std::string& mode) {
  if (mode == "processInPlace" || mode == "processThrough") {
    processorMode_ = mode;
  } else {
    processorMode_ = "processInPlace";
  }
}

// Registers a processor factory for a specific identifier
void CustomProcessorNode::registerProcessorFactory(
    const std::string& identifier,
    std::function<std::shared_ptr<CustomAudioProcessor>()> factory) {
  s_processorFactoriesByIdentifier[identifier] = std::move(factory);
  notifyProcessorChanged(identifier);
}

// Removes a processor factory for the specified identifier
void CustomProcessorNode::unregisterProcessorFactory(const std::string& identifier) {
  s_processorFactoriesByIdentifier.erase(identifier);
  notifyProcessorChanged(identifier);
}

// Notifies all active nodes with the given identifier to rebind their processor.
void CustomProcessorNode::notifyProcessorChanged(const std::string& identifier) {
  for (auto* node : activeNodes) {
    if (node->identifier_ == identifier) {
      auto it = s_processorFactoriesByIdentifier.find(identifier);
      node->processor_ = (it != s_processorFactoriesByIdentifier.end()) ? it->second() : nullptr;
    }
  }
}

// Registers a control handler callback for the specified identifier
void CustomProcessorNode::registerControlHandler(
    const std::string& identifier,
    GenericControlHandler handler) {
  s_controlHandlersByIdentifier[identifier] = std::move(handler);
}

// Unregisters the control handler associated with the given identifier
void CustomProcessorNode::unregisterControlHandler(const std::string& identifier) {
  s_controlHandlersByIdentifier.erase(identifier);
}

// Main processing entry point called during audio graph traversal
void CustomProcessorNode::processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) {
  if (!processor_) return;

  if (processorMode_ == "processThrough") {
    processThrough(processingBus, framesToProcess);
  } else {
    processInPlace(processingBus, framesToProcess);
  }
}

// In-place audio processing: modifies audio directly in memory
void CustomProcessorNode::processInPlace(const std::shared_ptr<AudioBus>& bus, int frames) {
  if (!processor_) return;

  int numChannels = bus->getNumberOfChannels();
  std::vector<float*> channelData(numChannels);

  for (int ch = 0; ch < numChannels; ++ch) {
    channelData[ch] = bus->getChannel(ch)->getData();
  }

  processor_->processInPlace(channelData.data(), numChannels, frames);
}

// Through-processing: processes audio with separate input/output buffers
void CustomProcessorNode::processThrough(const std::shared_ptr<AudioBus>& bus, int frames) {
  if (!processor_) return;

  int numChannels = bus->getNumberOfChannels();
  std::vector<float*> input(numChannels);
  std::vector<float*> output(numChannels);

  for (int ch = 0; ch < numChannels; ++ch) {
    input[ch] = bus->getChannel(ch)->getData();
    output[ch] = new float[frames];
  }

  processor_->processThrough(input.data(), output.data(), numChannels, frames);

  for (int ch = 0; ch < numChannels; ++ch) {
    std::memcpy(bus->getChannel(ch)->getData(), output[ch], sizeof(float) * frames);
    delete[] output[ch];
  }
}

} // namespace audioapi