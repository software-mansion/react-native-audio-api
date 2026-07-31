#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/channel_splitter/ChannelSplitterInputNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {

ChannelSplitterInputNode::ChannelSplitterInputNode(
    const std::shared_ptr<BaseAudioContext> &context,
    int numberOfChannels)
    : AudioNode(context, ChannelSplitterInputOptions(numberOfChannels)) {}

void ChannelSplitterInputNode::processNode(int framesToProcess) {
  // The bus (this node's output buffer) has already been filled by the base
  // processInputs (zero + discrete sum). Nothing else to do here.
  (void)framesToProcess;
}

} // namespace audioapi
