#include <audioapi/core/effects/WorkletNode.h>

namespace audioapi {

WorkletNode::WorkletNode(
    BaseAudioContext *context,
    std::shared_ptr<worklets::ShareableWorklet> &worklet,
    size_t bufferLength,
    size_t inputChannelCount)
    : AudioNode(context),
      bufferLength_(bufferLength),
      workletRunner_(context->workletRunner_),
      shareableWorklet_(worklet),
      inputChannelCount_(inputChannelCount),
      curBuffIndex_(0) {
  buffs_.reserve(inputChannelCount_);
  auto *runtime = workletRunner_->getJSIRuntime();
  for (size_t i = 0; i < inputChannelCount_; ++i) {
    buffs_.emplace_back(*runtime, bufferLength_);
  }
  isInitialized_ = true;
}

void WorkletNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t processed = 0;
  size_t channelCount_ = std::min(
      inputChannelCount_,
      static_cast<size_t>(processingBus->getNumberOfChannels()));
  while (processed < framesToProcess) {
    size_t framesToWorkletInvoke = bufferLength_ - curBuffIndex_;
    size_t needsToProcess = framesToProcess - processed;
    size_t shouldProcess = std::min(framesToWorkletInvoke, needsToProcess);
    auto uiRuntimeRaw = workletRunner_->getJSIRuntime();

    /// By creating an array when processing smaller chunks we distribute time
    /// that it takes to create bigger arrays
    for (size_t ch = 0; ch < channelCount_; ch++) {
      auto channelData = processingBus->getChannel(ch)->getData();
      auto &jsArray = buffs_[ch];
      for (size_t i = 0; i < shouldProcess; i++) {
        jsArray.setValueAtIndex(
            *uiRuntimeRaw,
            curBuffIndex_ + i,
            jsi::Value(channelData[processed + i]));
      }
    }
    processed += shouldProcess;
    curBuffIndex_ += shouldProcess;

    /// If we filled the entire buffer, we need to execute the worklet
    if (curBuffIndex_ == bufferLength_) {
      // Reset buffer index, channel buffers and execute worklet
      curBuffIndex_ = 0;
      auto jsArray = jsi::Array(*uiRuntimeRaw, channelCount_);
      for (size_t ch = 0; ch < channelCount_; ch++) {
        jsArray.setValueAtIndex(*uiRuntimeRaw, ch, buffs_[ch]);
        buffs_[ch] = jsi::Array(*uiRuntimeRaw, bufferLength_);
      }
      workletRunner_->executeWorkletSync(
          shareableWorklet_,
          jsArray,
          jsi::Value(*uiRuntimeRaw, static_cast<int>(channelCount_)));
    }
  }
}

} // namespace audioapi
