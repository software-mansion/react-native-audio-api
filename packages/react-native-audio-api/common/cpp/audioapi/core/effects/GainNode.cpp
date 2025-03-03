#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/GainNode.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

GainNode::GainNode(BaseAudioContext *context) : AudioNode(context) {
  gainParam_ = std::make_shared<AudioParam>(1.0, MIN_GAIN, MAX_GAIN);
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> GainNode::getGainParam() const {
  return gainParam_;
}

void GainNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  double time = context_->getCurrentTime();
  double deltaTime = 1.0 / context_->getSampleRate();

  for (int i = 0; i < framesToProcess; i += 1) {
    for (int j = 0; j < processingBus->getNumberOfChannels(); j += 1) {
      (*processingBus->getChannel(j))[i] *= gainParam_->getValueAtTime(time);
    }

    time += deltaTime;
  }
}

} // namespace audioapi
