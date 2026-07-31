#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/HostObjects/effects/ChannelSplitterNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/channel_splitter/ChannelSplitterInputNode.h>
#include <audioapi/core/effects/channel_splitter/ChannelSplitterOutputNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

ChannelSplitterNodeHostObject::ChannelSplitterNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const ChannelSplitterOptions &options)
    : AudioNodeHostObject(
          context->getGraph(),
          std::make_unique<ChannelSplitterInputNode>(context, options.numberOfOutputs),
          options),
      inputNode_(typedAudioNode<ChannelSplitterInputNode>(node_)) {
  // The input node's buffer is the shared bus that each output reads from.
  auto bus = inputNode_->getOutputBuffer();

  outputHostNodes_.reserve(static_cast<size_t>(options.numberOfOutputs));
  for (int i = 0; i < options.numberOfOutputs; ++i) {
    auto outputNode =
        std::make_unique<ChannelSplitterOutputNode>(context, bus, static_cast<size_t>(i));
    auto outputHostNode = std::make_shared<utils::graph::HostNode>(graph_, std::move(outputNode));

    // Edge input → output, so the input (which fills the bus) is always
    // processed before every output that reads from it.
    connect(*outputHostNode);

    outputHostNodes_.push_back(std::move(outputHostNode));
  }
}

std::shared_ptr<utils::graph::HostNode> ChannelSplitterNodeHostObject::getOutput(int outputIndex) {
  return outputHostNodes_[static_cast<size_t>(outputIndex)];
}

size_t ChannelSplitterNodeHostObject::getMemoryPressure() const {
  // Base input node buffer + one mono RQ buffer per output host node.
  return AudioNodeHostObject::getMemoryPressure() +
      outputHostNodes_.size() * RENDER_QUANTUM_SIZE * sizeof(float);
}

} // namespace audioapi
