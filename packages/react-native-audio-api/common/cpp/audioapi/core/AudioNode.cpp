#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <memory>

namespace audioapi {

AudioNode::AudioNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioNodeOptions &options)
    : context_(context),
      numberOfInputs_(options.numberOfInputs),
      numberOfOutputs_(options.numberOfOutputs),
      channelCount_(options.channelCount),
      channelCountMode_(options.channelCountMode),
      channelInterpretation_(options.channelInterpretation),
      requiresTailProcessing_(options.requiresTailProcessing) {
  audioBuffer_ = std::make_shared<DSPAudioBuffer>(
      RENDER_QUANTUM_SIZE, channelCount_, context->getSampleRate());
}

bool AudioNode::canBeDestructed() const {
  return true;
}

size_t AudioNode::getChannelCount() const {
  return channelCount_;
}

bool AudioNode::requiresTailProcessing() const {
  return requiresTailProcessing_;
}

} // namespace audioapi
