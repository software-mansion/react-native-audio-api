#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/channel_merger/ChannelMergerInputNode.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <cstddef>
#include <memory>
#include <utility>

namespace audioapi {

namespace {
AudioNodeOptions makeInputOptions() {
  AudioNodeOptions options;
  options.numberOfInputs = 1;
  options.numberOfOutputs = 1;
  options.channelCount = 1;
  options.channelCountMode = ChannelCountMode::EXPLICIT;
  options.channelInterpretation = ChannelInterpretation::SPEAKERS;
  return options;
}
} // namespace

ChannelMergerInputNode::ChannelMergerInputNode(
    const std::shared_ptr<BaseAudioContext> &context,
    std::shared_ptr<DSPAudioBuffer> bus,
    size_t channelIndex)
    : AudioNode(context, makeInputOptions()), bus_(std::move(bus)), channelIndex_(channelIndex) {}

void ChannelMergerInputNode::processNode(int framesToProcess) {
  // getInputBuffer() has already summed and downmixed every upstream
  // connection to a single (mono) channel. Copy it into this input's slot in
  // the shared bus.
  bus_->getChannel(channelIndex_)
      ->copy(*getInputBuffer()->getChannel(0), 0, 0, static_cast<size_t>(framesToProcess));
}

} // namespace audioapi
