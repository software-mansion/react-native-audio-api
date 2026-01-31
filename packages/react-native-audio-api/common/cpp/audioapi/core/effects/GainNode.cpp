#include <audioapi/types/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/GainNode.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <memory>

namespace audioapi {

GainNode::GainNode(const std::shared_ptr<BaseAudioContext> &context, const GainOptions &options)
    : AudioNode(context, options),
      gainParam_(
          std::make_shared<AudioParam>(
              options.gain,
              MOST_NEGATIVE_SINGLE_FLOAT,
              MOST_POSITIVE_SINGLE_FLOAT,
              context)) {
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> GainNode::getGainParam() const {
  return gainParam_;
}

std::shared_ptr<AudioBus> GainNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr)
    return processingBus;
  double time = context->getCurrentTime();
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
