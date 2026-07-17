#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/HostObjects/effects/ChannelMergerNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/channel_merger/ChannelMergerInputNode.h>
#include <audioapi/core/effects/channel_merger/ChannelMergerOutputNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

ChannelMergerNodeHostObject::ChannelMergerNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const ChannelMergerOptions &options)
    : AudioNodeHostObject(
          context->getGraph(),
          std::make_unique<ChannelMergerOutputNode>(context, options.numberOfInputs),
          options),
      outputNode_(typedAudioNode<ChannelMergerOutputNode>(node_)) {
  // The output node's buffer is the shared bus that every input writes into.
  auto bus = outputNode_->getOutputBuffer();

  inputHostNodes_.reserve(static_cast<size_t>(options.numberOfInputs));
  for (int i = 0; i < options.numberOfInputs; ++i) {
    auto inputNode = std::make_unique<ChannelMergerInputNode>(context, bus, static_cast<size_t>(i));
    auto inputHostNode = std::make_shared<utils::graph::HostNode>(graph_, std::move(inputNode));

    // Edge input → output, so the inputs are always processed before the
    // output node that reads the assembled bus.
    inputHostNode->connect(*this);

    inputHostNodes_.push_back(std::move(inputHostNode));
  }
}

std::shared_ptr<utils::graph::HostNode> ChannelMergerNodeHostObject::getConnectDestination(
    int inputIndex) {
  return inputHostNodes_[static_cast<size_t>(inputIndex)];
}

size_t ChannelMergerNodeHostObject::getMemoryPressure() const {
  // Base output node buffer + one mono RQ buffer per input host node.
  return AudioNodeHostObject::getMemoryPressure() +
      inputHostNodes_.size() * RENDER_QUANTUM_SIZE * sizeof(float);
}

} // namespace audioapi
