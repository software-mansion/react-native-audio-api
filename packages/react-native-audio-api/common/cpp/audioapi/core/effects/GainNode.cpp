#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/GainNode.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

GainNode::GainNode(BaseAudioContext *context) : AudioNode(context) {
  gainParam_ = std::make_shared<AudioParam>(
      1.0, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> GainNode::getGainParam() const {
  return gainParam_;
}

std::shared_ptr<AudioBus> GainNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  double time = context_->getCurrentTime();
  auto gainParamValues = gainParam_->processARateParam(framesToProcess, time);
  for (int i = 0; i < processingBus->getNumberOfChannels(); i += 1) {
    dsp::multiply(
        processingBus->getChannel(i)->getData(),
        gainParamValues->getChannel(0)->getData(),
        processingBus->getChannel(i)->getData(),
        framesToProcess);
  }

  return processingBus;
}

} // namespace audioapi
