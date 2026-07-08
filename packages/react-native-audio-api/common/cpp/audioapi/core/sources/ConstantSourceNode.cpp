#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <memory>

namespace audioapi {
ConstantSourceNode::ConstantSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const ConstantSourceOptions &options)
    : AudioScheduledSourceNode(context),
      offsetParam_(
          std::make_shared<AudioParam>(
              options.offset,
              MOST_NEGATIVE_SINGLE_FLOAT,
              MOST_POSITIVE_SINGLE_FLOAT,
              context)) {}

std::shared_ptr<AudioParam> ConstantSourceNode::getOffsetParam() const {
  return offsetParam_;
}

void ConstantSourceNode::processNode(int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    audioBuffer_->zero();
    return;
  }

  updatePlaybackInfo(
      audioBuffer_,
      framesToProcess,
      startOffset,
      offsetLength,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (!isPlaying() && !isStopScheduled()) {
    audioBuffer_->zero();
    return;
  }

  auto *offsetChannel =
      offsetParam_->processARateParam(framesToProcess, context->getCurrentTime())->getChannel(0);

  for (size_t channel = 0; channel < audioBuffer_->getNumberOfChannels(); ++channel) {
    audioBuffer_->getChannel(channel)->copy(*offsetChannel, startOffset, startOffset, offsetLength);
  }

  if (isStopScheduled()) {
    handleStopScheduled();
  }
}
} // namespace audioapi
