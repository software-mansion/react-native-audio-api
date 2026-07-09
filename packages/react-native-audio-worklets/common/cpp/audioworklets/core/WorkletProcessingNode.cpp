#include <audioworklets/core/WorkletProcessingNode.h>

#include <jsi/jsi.h>

#include <memory>
#include <utility>

namespace audioworklets {

WorkletProcessingNode::WorkletProcessingNode(
    const std::shared_ptr<audioapi::BaseAudioContext> &context,
    AudioWorkletsRunner &&workletRunner)
    : audioapi::AudioNode(context), workletRunner_(std::move(workletRunner)) {
  const auto frameCount = audioapi::RENDER_QUANTUM_SIZE;
  const auto channelCount = static_cast<size_t>(audioapi::MAX_CHANNEL_COUNT);

  inputChannelViews_ = workletRunner_.createChannelViews(frameCount, channelCount);
  outputChannelViews_ = workletRunner_.createChannelViews(frameCount, channelCount);
}

WorkletProcessingNode::~WorkletProcessingNode() {
  if (inputChannelViews_ != nullptr) {
    inputChannelViews_->releaseJsValues();
  }
  if (outputChannelViews_ != nullptr) {
    outputChannelViews_->releaseJsValues();
  }
}

void WorkletProcessingNode::processNode(int framesToProcess) {
  if (inputChannelViews_ == nullptr || outputChannelViews_ == nullptr || framesToProcess <= 0) {
    return;
  }

  const size_t inputChannelCount = getInputBuffer()->getNumberOfChannels();
  const size_t outputChannelCount = getOutputBuffer()->getNumberOfChannels();

  if (inputChannelCount == 0 || outputChannelCount == 0) {
    return;
  }

  for (size_t ch = 0; ch < inputChannelCount; ++ch) {
    inputChannelViews_->channelBuffer(ch)->copy(
        *getInputBuffer()->getChannel(ch), 0, 0, framesToProcess);
  }

  double time = 0.0;
  if (std::shared_ptr<audioapi::BaseAudioContext> context = context_.lock()) {
    time = context->getCurrentTime();
  }

  const jsi::Value *inputData = inputChannelViews_->channelsArray(inputChannelCount);
  const jsi::Value *outputData = outputChannelViews_->channelsArray(outputChannelCount);
  if (inputData == nullptr || outputData == nullptr) {
    for (size_t ch = 0; ch < outputChannelCount; ++ch) {
      getOutputBuffer()->getChannel(ch)->zero(0, framesToProcess);
    }
    return;
  }

  auto result = workletRunner_.call(
      *inputData,
      *outputData,
      jsi::Value(static_cast<int>(inputChannelCount)),
      jsi::Value(static_cast<int>(outputChannelCount)),
      jsi::Value(framesToProcess),
      jsi::Value(time));

  for (size_t ch = 0; ch < outputChannelCount; ++ch) {
    auto *channelData = getOutputBuffer()->getChannel(ch);

    if (result.has_value()) {
      channelData->copy(*outputChannelViews_->channelBuffer(ch), 0, 0, framesToProcess);
    } else {
      channelData->zero(0, framesToProcess);
    }
  }
}

} // namespace audioworklets
