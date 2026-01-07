#include <audioapi/core/AudioNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/AudioNodeManager.h>
#include <audioapi/utils/AudioBus.h>
#include <memory>

namespace audioapi {

AudioDestinationNode::AudioDestinationNode(std::shared_ptr<BaseAudioContext> context)
    : AudioNode(context), currentSampleFrame_(0) {
  numberOfOutputs_ = 0;
  numberOfInputs_ = 1;
  channelCountMode_ = ChannelCountMode::EXPLICIT;
  isInitialized_ = true;
}

std::size_t AudioDestinationNode::getCurrentSampleFrame() const {
  return currentSampleFrame_;
}

double AudioDestinationNode::getCurrentTime() const {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    return static_cast<double>(currentSampleFrame_) / context->getSampleRate();
  } else {
    return 0.0;
  }
}

void AudioDestinationNode::renderAudio(
    const std::shared_ptr<AudioBus> &destinationBus,
    int numFrames) {
  if (numFrames < 0 || !destinationBus || !isInitialized_) {
    return;
  }

  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    context->getNodeManager()->preProcessGraph();
  }

  destinationBus->zero();

  auto processedBus = processAudio(destinationBus, numFrames, true);

  if (processedBus && processedBus != destinationBus) {
    destinationBus->copy(processedBus.get());
  }

  destinationBus->normalize();

  currentSampleFrame_ += numFrames;
}

} // namespace audioapi
