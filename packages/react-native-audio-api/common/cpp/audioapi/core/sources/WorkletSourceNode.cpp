#include <audioapi/core/sources/WorkletSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <memory>
#include <utility>

namespace audioapi {

WorkletSourceNode::WorkletSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    WorkletsRunner &&workletRunner)
    : AudioScheduledSourceNode(context), workletRunner_(std::move(workletRunner)) {
  isInitialized_ = true;

  // Prepare buffers for audio processing
  size_t outputChannelCount = this->getChannelCount();
  outputBuffsHandles_.resize(outputChannelCount);
  for (size_t i = 0; i < outputChannelCount; ++i) {
    outputBuffsHandles_[i] = std::make_shared<AudioArrayBuffer>(RENDER_QUANTUM_SIZE);
  }
}

std::shared_ptr<AudioBuffer> WorkletSourceNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (isUnscheduled() || isFinished() || !isEnabled()) {
    processingBuffer->zero();
    return processingBuffer;
  }

  size_t startOffset = 0;
  size_t nonSilentFramesToProcess = framesToProcess;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }
  updatePlaybackInfo(
      processingBuffer,
      framesToProcess,
      startOffset,
      nonSilentFramesToProcess,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (nonSilentFramesToProcess == 0) {
    processingBuffer->zero();
    return processingBuffer;
  }

  size_t outputChannelCount = processingBuffer->getNumberOfChannels();

  auto result = workletRunner_.executeOnRuntimeSync(
      [this, nonSilentFramesToProcess, startOffset, time = context->getCurrentTime()](
          jsi::Runtime &rt) {
        auto jsiArray = jsi::Array(rt, this->outputBuffsHandles_.size());
        for (size_t i = 0; i < this->outputBuffsHandles_.size(); ++i) {
          auto arrayBuffer = jsi::ArrayBuffer(rt, this->outputBuffsHandles_[i]);
          jsiArray.setValueAtIndex(rt, i, arrayBuffer);
        }

        // We call unsafely here because we are already on the runtime thread
        // and the runtime is locked by executeOnRuntimeSync (if
        // shouldLockRuntime is true)
        return workletRunner_.callUnsafe(
            jsiArray,
            jsi::Value(rt, static_cast<int>(nonSilentFramesToProcess)),
            jsi::Value(rt, time),
            jsi::Value(rt, static_cast<int>(startOffset)));
      });

  // If the worklet execution failed, zero the output
  // It might happen if the runtime is not available
  if (!result.has_value()) {
    processingBuffer->zero();
    return processingBuffer;
  }

  // Copy the processed data back to the AudioBuffer
  for (size_t i = 0; i < outputChannelCount; ++i) {
    processingBuffer->getChannel(i)->copy(
        *outputBuffsHandles_[i], 0, startOffset, nonSilentFramesToProcess);
  }

  handleStopScheduled();

  return processingBuffer;
}

} // namespace audioapi
