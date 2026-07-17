#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/channel_merger/ChannelMergerOutputNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <vector>

namespace audioapi {

namespace {
AudioNodeOptions makeOutputOptions(int numberOfChannels) {
  AudioNodeOptions options;
  options.numberOfInputs = numberOfChannels;
  options.numberOfOutputs = 1;
  options.channelCount = numberOfChannels;
  options.channelCountMode = ChannelCountMode::EXPLICIT;
  options.channelInterpretation = ChannelInterpretation::DISCRETE;
  return options;
}
} // namespace

ChannelMergerOutputNode::ChannelMergerOutputNode(
    const std::shared_ptr<BaseAudioContext> &context,
    int numberOfChannels)
    : AudioNode(context, makeOutputOptions(numberOfChannels)) {}

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
