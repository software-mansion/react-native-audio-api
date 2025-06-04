#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/CustomProcessorNode.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

#include <iostream>  // Required for console output

namespace audioapi {

CustomProcessorNode::CustomProcessorNode(BaseAudioContext *context)
    : AudioNode(context) {
  customProcessorParam_ = std::make_shared<AudioParam>(
      1.0f, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> CustomProcessorNode::getCustomProcessorParam() const {
  return customProcessorParam_;
}

std::string CustomProcessorNode::getIdentifier() const {
  return identifier_;
}

void CustomProcessorNode::setIdentifier(const std::string& identifier) {
  identifier_ = identifier;
}

void CustomProcessorNode::processNode(
    const std::shared_ptr<AudioBus>& processingBus,
    int framesToProcess) {

  double time = context_->getCurrentTime();
  auto customProcessorParamValues = customProcessorParam_->processARateParam(framesToProcess, time);

  // ✅ Print identifier and the first value in param buffer
  std::cout << "[CustomProcessorNode] Identifier: " << identifier_ << std::endl;
  if (customProcessorParamValues && customProcessorParamValues->getChannel(0)) {
    float firstValue = (*customProcessorParamValues->getChannel(0))[0];
    std::cout << "[CustomProcessorNode] First Param Value: " << firstValue << std::endl;
  }

  for (int i = 0; i < processingBus->getNumberOfChannels(); ++i) {
    dsp::multiply(
        processingBus->getChannel(i)->getData(),
        customProcessorParamValues->getChannel(0)->getData(),
        processingBus->getChannel(i)->getData(),
        framesToProcess);
  }
}

} // namespace audioapi