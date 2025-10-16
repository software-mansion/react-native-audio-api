#include <audioapi/core/sources/WorkletSourceNode.h>
#include <audioapi/core/utils/Constants.h>

namespace audioapi {

WorkletSourceNode::WorkletSourceNode(
    BaseAudioContext *context,
    WorkletsRunner &&workletRunner)
    : AudioScheduledSourceNode(context),
      workletRunner_(std::move(workletRunner)) {
  isInitialized_ = true;

  // Prepare buffers for audio processing
  size_t outputChannelCount = this->getChannelCount();
  outputBuffsHandles_.resize(outputChannelCount);
  for (size_t i = 0; i < outputChannelCount; ++i) {
    auto audioArray = std::make_shared<AudioArray>(RENDER_QUANTUM_SIZE);
    outputBuffsHandles_[i] = std::make_shared<AudioArrayBuffer>(audioArray);
  }
}

std::shared_ptr<AudioBus> WorkletSourceNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  if (isUnscheduled() || isFinished() || !isEnabled()) {
    processingBus->zero();
    return processingBus;
  }

  size_t startOffset = 0;
  size_t nonSilentFramesToProcess = framesToProcess;

  updatePlaybackInfo(
      processingBus, framesToProcess, startOffset, nonSilentFramesToProcess);

  if (nonSilentFramesToProcess == 0) {
    processingBus->zero();
    return processingBus;
  }

  size_t outputChannelCount = processingBus->getNumberOfChannels();

  auto result = workletRunner_.executeOnRuntimeSync(
      [this, nonSilentFramesToProcess, startOffset](jsi::Runtime &rt) {
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
            jsi::Value(rt, this->context_->getCurrentTime()),
            jsi::Value(rt, static_cast<int>(startOffset)));
      });

  // If the worklet execution failed, zero the output
  // It might happen if the runtime is not available
  if (!result.has_value()) {
    processingBus->zero();
    return processingBus;
  }

  // Copy the processed data back to the AudioBus
  for (size_t i = 0; i < outputChannelCount; ++i) {
    float *channelData = processingBus->getChannel(i)->getData();
    memcpy(
        channelData + startOffset,
        outputBuffsHandles_[i]->data(),
        nonSilentFramesToProcess * sizeof(float));
  }

  handleStopScheduled();

  return processingBus;
}

} // namespace audioapi
