#include <audioapi/types/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/GainNode.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
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

std::shared_ptr<AudioBuffer> GainNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr)
    return processingBuffer;
  double time = context->getCurrentTime();
  auto gainParamValues = gainParam_->processARateParam(framesToProcess, time);
  auto gainValues = gainParamValues->getChannel(0);

  for (size_t i = 0; i < processingBuffer->getNumberOfChannels(); i++) {
    auto channel = processingBuffer->getChannel(i);
    channel->multiply(*gainValues, framesToProcess);
  }

  return processingBuffer;
}

} // namespace audioapi
