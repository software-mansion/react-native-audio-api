#include <audioworklets/core/WorkletSourceNode.h>

#include <jsi/jsi.h>

#include <memory>
#include <utility>

namespace audioworklets {

WorkletSourceNode::WorkletSourceNode(
    const std::shared_ptr<audioapi::BaseAudioContext> &context,
    AudioWorkletsRunner &&workletRunner)
    : audioapi::AudioScheduledSourceNode(context), workletRunner_(std::move(workletRunner)) {
  outputChannelViews_ = workletRunner_.createChannelViews(
      audioapi::RENDER_QUANTUM_SIZE, static_cast<size_t>(audioapi::MAX_CHANNEL_COUNT));
}

WorkletSourceNode::~WorkletSourceNode() {
  if (outputChannelViews_ != nullptr) {
    outputChannelViews_->releaseJsValues();
  }
}

void WorkletSourceNode::processNode(int framesToProcess) {
  if (outputChannelViews_ == nullptr || framesToProcess <= 0) {
    return;
  }

  if (isUnscheduled() || isFinished()) {
    audioBuffer_->zero();
    return;
  }

  size_t startOffset = 0;
  auto nonSilentFramesToProcess = static_cast<size_t>(framesToProcess);

  std::shared_ptr<audioapi::BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    audioBuffer_->zero();
    return;
  }

  updatePlaybackInfo(
      audioBuffer_,
      framesToProcess,
      startOffset,
      nonSilentFramesToProcess,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if ((!isPlaying() && !isStopScheduled()) || nonSilentFramesToProcess == 0) {
    audioBuffer_->zero();
    return;
  }

  const size_t outputChannelCount = audioBuffer_->getNumberOfChannels();
  const jsi::Value *outputData = outputChannelViews_->channelsArray(outputChannelCount);
  if (outputData == nullptr || !workletRunner_.isActive()) {
    audioBuffer_->zero();
    return;
  }

  workletRunner_.callUnsafe(
      *outputData,
      jsi::Value(static_cast<int>(outputChannelCount)),
      jsi::Value(static_cast<int>(nonSilentFramesToProcess)),
      jsi::Value(context->getCurrentTime()),
      jsi::Value(static_cast<int>(startOffset)));

  for (size_t i = 0; i < outputChannelCount; ++i) {
    audioBuffer_->getChannel(i)->copy(
        *outputChannelViews_->channelBuffer(i), 0, startOffset, nonSilentFramesToProcess);
  }

  handleStopScheduled();
}

} // namespace audioworklets
