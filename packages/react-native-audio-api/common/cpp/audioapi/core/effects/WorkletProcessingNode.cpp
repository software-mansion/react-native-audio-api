#include <audioapi/core/effects/WorkletProcessingNode.h>
#include <audioapi/core/utils/Constants.h>
#include <memory>
#include <utility>

namespace audioapi {

WorkletProcessingNode::WorkletProcessingNode(
    const std::shared_ptr<BaseAudioContext> &context,
    WorkletsRunner &&workletRunner)
    : AudioNode(context), workletRunner_(std::move(workletRunner)) {
  // Pre-allocate buffers for max 128 frames and 2 channels (stereo)
  size_t maxChannelCount = 2;
  inputBuffsHandles_.resize(maxChannelCount);
  outputBuffsHandles_.resize(maxChannelCount);

  for (size_t i = 0; i < maxChannelCount; ++i) {
    inputBuffsHandles_[i] = std::make_shared<AudioArrayBuffer>(RENDER_QUANTUM_SIZE);
    outputBuffsHandles_[i] = std::make_shared<AudioArrayBuffer>(RENDER_QUANTUM_SIZE);
  }
  isInitialized_ = true;
}

std::shared_ptr<AudioBuffer> WorkletProcessingNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  size_t channelCount = std::min(
      static_cast<size_t>(2), // Fixed to stereo for now
      static_cast<size_t>(processingBuffer->getNumberOfChannels()));

  // Copy input data to pre-allocated input buffers
  for (size_t ch = 0; ch < channelCount; ch++) {
    inputBuffsHandles_[ch]->copy(*processingBuffer->getChannel(ch), 0, 0, framesToProcess);
  }

  // Execute the worklet
  auto result = workletRunner_.executeOnRuntimeSync(
      [this, channelCount, framesToProcess](jsi::Runtime &rt) -> jsi::Value {
        auto inputJsArray = jsi::Array(rt, channelCount);
        auto outputJsArray = jsi::Array(rt, channelCount);

        for (size_t ch = 0; ch < channelCount; ch++) {
          // Create input array buffer
          auto inputArrayBuffer = jsi::ArrayBuffer(rt, inputBuffsHandles_[ch]);
          inputJsArray.setValueAtIndex(rt, ch, inputArrayBuffer);

          // Create output array buffer
          auto outputArrayBuffer = jsi::ArrayBuffer(rt, outputBuffsHandles_[ch]);
          outputJsArray.setValueAtIndex(rt, ch, outputArrayBuffer);
        }

        // We call unsafely here because we are already on the runtime thread
        // and the runtime is locked by executeOnRuntimeSync (if
        // shouldLockRuntime is true)
        float time = 0.0f;
        if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
          time = context->getCurrentTime();
        }
        return workletRunner_.callUnsafe(
            inputJsArray,
            outputJsArray,
            jsi::Value(rt, static_cast<int>(framesToProcess)),
            jsi::Value(rt, time));
      });

  // Copy processed output data back to the processing buffer or zero on failure
  for (size_t ch = 0; ch < channelCount; ch++) {
    auto channelData = processingBuffer->getChannel(ch);

    if (result.has_value()) {
      // Copy processed output data
      channelData->copy(*inputBuffsHandles_[ch], 0, 0, framesToProcess);
    } else {
      // Zero the output on worklet execution failure
      channelData->zero(0, framesToProcess);
    }
  }

  return processingBuffer;
}

} // namespace audioapi
