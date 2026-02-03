#include <audioapi/core/AudioNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <audioapi/utils/AudioBus.h>
#include <memory>

namespace audioapi {

AudioDestinationNode::AudioDestinationNode(const std::shared_ptr<BaseAudioContext> &context)
    : AudioNode(context), currentSampleFrame_(0) {
  numberOfOutputs_ = 0;
  numberOfInputs_ = 1;
  channelCountMode_ = ChannelCountMode::EXPLICIT;
  isInitialized_ = true;
}

std::size_t AudioDestinationNode::getCurrentSampleFrame() const {
  return currentSampleFrame_.load(std::memory_order_acquire);
}

double AudioDestinationNode::getCurrentTime() const {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    return static_cast<double>(getCurrentSampleFrame()) / context->getSampleRate();
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
      context->getGraphManager()->preProcessGraph();
  }

  destinationBus->zero();

  auto processedBus = processAudio(destinationBus, numFrames, true);

  if (processedBus && processedBus != destinationBus) {
    destinationBus->copy(processedBus.get());
  }

  destinationBus->normalize();

  currentSampleFrame_.fetch_add(numFrames, std::memory_order_release);
}

} // namespace audioapi
