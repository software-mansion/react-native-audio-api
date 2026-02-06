#include <audioapi/types/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
#include <memory>

namespace audioapi {
ConstantSourceNode::ConstantSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const ConstantSourceOptions &options)
    : AudioScheduledSourceNode(context) {
  offsetParam_ = std::make_shared<AudioParam>(
      options.offset, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> ConstantSourceNode::getOffsetParam() const {
  return offsetParam_;
}

std::shared_ptr<AudioBuffer> ConstantSourceNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }

  updatePlaybackInfo(
      processingBuffer,
      framesToProcess,
      startOffset,
      offsetLength,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (!isPlaying() && !isStopScheduled()) {
    processingBuffer->zero();
    return processingBuffer;
  }

  auto offsetChannel =
      offsetParam_->processARateParam(framesToProcess, context->getCurrentTime())->getChannel(0);

  for (size_t channel = 0; channel < processingBuffer->getNumberOfChannels(); ++channel) {
    processingBuffer->getChannel(channel)->copy(
        *offsetChannel, startOffset, startOffset, offsetLength);
  }

  if (isStopScheduled()) {
    handleStopScheduled();
  }

  return processingBuffer;
}
} // namespace audioapi
