#include <audioapi/core/effects/WorkletNode.h>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

WorkletNode::WorkletNode(
    const std::shared_ptr<BaseAudioContext> &context,
    size_t bufferLength,
    size_t inputChannelCount,
    WorkletsRunner &&runtime)
    : AudioNode(context),
      workletRunner_(std::move(runtime)),
      buffer_(
          std::make_shared<AudioBuffer>(bufferLength, inputChannelCount, context->getSampleRate())),
      bufferLength_(bufferLength),
      inputChannelCount_(inputChannelCount),
      curBuffIndex_(0) {
  isInitialized_ = true;
}

std::shared_ptr<AudioBuffer> WorkletNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  size_t processed = 0;
  size_t channelCount_ =
      std::min(inputChannelCount_, static_cast<size_t>(processingBuffer->getNumberOfChannels()));
  while (processed < framesToProcess) {
    size_t framesToWorkletInvoke = bufferLength_ - curBuffIndex_;
    size_t needsToProcess = framesToProcess - processed;
    size_t shouldProcess = std::min(framesToWorkletInvoke, needsToProcess);

    /// here we copy
    /// to [curBuffIndex_, curBuffIndex_ + shouldProcess]
    /// from [processed, processed + shouldProcess]
    buffer_->copy(*processingBuffer, processed, curBuffIndex_, shouldProcess);

    processed += shouldProcess;
    curBuffIndex_ += shouldProcess;

    /// If we filled the entire buffer, we need to execute the worklet
    if (curBuffIndex_ != bufferLength_) {
      continue;
    }
    // Reset buffer index, channel buffers and execute worklet
    curBuffIndex_ = 0;
    workletRunner_.executeOnRuntimeSync([this, channelCount_](jsi::Runtime &uiRuntimeRaw) {
      /// Arguments preparation
      auto jsArray = jsi::Array(uiRuntimeRaw, channelCount_);
      for (size_t ch = 0; ch < channelCount_; ch++) {
        auto sharedAudioArray = std::make_shared<AudioArrayBuffer>(bufferLength_);
        sharedAudioArray->copy(*buffer_->getChannel(ch));
        auto sharedAudioArraySize = sharedAudioArray->size();
        auto arrayBuffer = jsi::ArrayBuffer(uiRuntimeRaw, std::move(sharedAudioArray));
        arrayBuffer.setExternalMemoryPressure(uiRuntimeRaw, sharedAudioArraySize);
        jsArray.setValueAtIndex(uiRuntimeRaw, ch, std::move(arrayBuffer));
      }

      buffer_->zero();

      /// Call the worklet
      workletRunner_.callUnsafe(
          std::move(jsArray), jsi::Value(uiRuntimeRaw, static_cast<int>(channelCount_)));

      return jsi::Value::undefined();
    });
  }

  return processingBuffer;
}

} // namespace audioapi
