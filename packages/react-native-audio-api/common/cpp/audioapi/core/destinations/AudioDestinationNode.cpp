#include <audioapi/core/AudioNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.h>
#include <memory>

namespace audioapi {

AudioDestinationNode::AudioDestinationNode(const std::shared_ptr<BaseAudioContext> &context)
    : AudioNode(context, AudioDestinationOptions()), currentSampleFrame_(0) {
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
    const std::shared_ptr<AudioBuffer> &destinationBuffer,
    int numFrames) {
  if (numFrames < 0 || !destinationBuffer || !isInitialized_) {
    return;
  }

  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    context->getGraphManager()->preProcessGraph();
  }

  destinationBuffer->zero();

  auto processedBuffer = processAudio(destinationBuffer, numFrames, true);

  if (processedBuffer && processedBuffer != destinationBuffer) {
    destinationBuffer->copy(*processedBuffer);
  }

  destinationBuffer->normalize();

  currentSampleFrame_.fetch_add(numFrames, std::memory_order_release);
}

} // namespace audioapi
