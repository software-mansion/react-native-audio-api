#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/channel_splitter/ChannelSplitterOutputNode.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <cstddef>
#include <memory>
#include <utility>

namespace audioapi {

ChannelSplitterOutputNode::ChannelSplitterOutputNode(
    const std::shared_ptr<BaseAudioContext> &context,
    std::shared_ptr<DSPAudioBuffer> bus,
    size_t channelIndex)
    : AudioNode(context, ChannelSplitterOutputOptions()),
      bus_(std::move(bus)),
      channelIndex_(channelIndex) {}

void ChannelSplitterOutputNode::processNode(int framesToProcess) {
  // getInputBuffer() was already zeroed by the base processInputs, so an
  // inactive channel naturally emits silence.
  if (channelIndex_ >= bus_->getNumberOfChannels()) {
    return;
  }

  getOutputBuffer()->getChannel(0)->copy(
      *bus_->getChannel(channelIndex_), 0, 0, static_cast<size_t>(framesToProcess));
}

} // namespace audioapi
