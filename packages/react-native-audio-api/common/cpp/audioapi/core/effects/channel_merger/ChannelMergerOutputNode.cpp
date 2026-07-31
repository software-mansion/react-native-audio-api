#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/channel_merger/ChannelMergerOutputNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <vector>

namespace audioapi {

ChannelMergerOutputNode::ChannelMergerOutputNode(
    const std::shared_ptr<BaseAudioContext> &context,
    int numberOfChannels)
    : AudioNode(context, ChannelMergerOutputOptions(numberOfChannels)) {}

void ChannelMergerOutputNode::processInputs(
    const std::vector<const DSPAudioBuffer *> &inputs,
    int numFrames) {
  // Intentionally empty: the bus (this node's output buffer) is filled by the
  // input nodes, which are guaranteed to run earlier in the topological order.
  // Zeroing or summing here would clobber that data.
  (void)inputs;
  (void)numFrames;
}

void ChannelMergerOutputNode::processNode(int framesToProcess) {
  (void)framesToProcess;
}

} // namespace audioapi
