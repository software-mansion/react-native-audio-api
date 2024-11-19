#include "GainNode.h"
#include "AudioBus.h"
#include "AudioArray.h"
#include "BaseAudioContext.h"

namespace audioapi {

GainNode::GainNode(BaseAudioContext *context) : AudioNode(context) {
  gainParam_ = std::make_shared<AudioParam>(context, 1.0, -MAX_GAIN, MAX_GAIN);
}

std::shared_ptr<AudioParam> GainNode::getGainParam() const {
  return gainParam_;
}

void GainNode::processNode(AudioBus *processingBus, int framesToProcess) {
  double time = context_->getCurrentTime();
  double deltaTime = 1.0 / context_->getSampleRate();

  for (int i = 0; i < framesToProcess; i += 1) {
    for (int j = 0; j < processingBus->getNumberOfChannels(); j += 1) {
      (*processingBus->getChannel(j))[i] *= gainParam_->getValueAtTime(time);
    }
  }
}

} // namespace audioapi
