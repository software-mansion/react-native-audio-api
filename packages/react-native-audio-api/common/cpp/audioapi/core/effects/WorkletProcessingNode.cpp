#include <audioapi/core/effects/WorkletProcessingNode.h>
#include <audioapi/core/utils/Constants.h>

namespace audioapi {

WorkletProcessingNode::WorkletProcessingNode(
    BaseAudioContext *context,
    WorkletsRunner &&workletRunner)
    : AudioNode(context), workletRunner_(std::move(workletRunner)) {
  isInitialized_ = true;

  // Pre-allocate buffers for max 128 frames and 2 channels (stereo)
  size_t maxChannelCount = 2;
  inputBuffsHandles_.resize(maxChannelCount);
  outputBuffsHandles_.resize(maxChannelCount);

  for (size_t i = 0; i < maxChannelCount; ++i) {
    auto inputAudioArray = std::make_shared<AudioArray>(RENDER_QUANTUM_SIZE);
    inputBuffsHandles_[i] = std::make_shared<AudioArrayBuffer>(inputAudioArray);

    auto outputAudioArray = std::make_shared<AudioArray>(RENDER_QUANTUM_SIZE);
    outputBuffsHandles_[i] =
        std::make_shared<AudioArrayBuffer>(outputAudioArray);
  }
}

std::shared_ptr<AudioBus> WorkletProcessingNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t channelCount = std::min(
      static_cast<size_t>(2), // Fixed to stereo for now
      static_cast<size_t>(processingBus->getNumberOfChannels()));

  // Copy input data to pre-allocated input buffers
  for (size_t ch = 0; ch < channelCount; ch++) {
    auto channelData = processingBus->getChannel(ch)->getData();
    std::memcpy(
        /* dest */ inputBuffsHandles_[ch]->data(),
        /* src */ reinterpret_cast<const uint8_t *>(channelData),
        /* size */ framesToProcess * sizeof(float));
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
          auto outputArrayBuffer =
              jsi::ArrayBuffer(rt, outputBuffsHandles_[ch]);
          outputJsArray.setValueAtIndex(rt, ch, outputArrayBuffer);
        }

        // We call unsafely here because we are already on the runtime thread
        // and the runtime is locked by executeOnRuntimeSync (if
        // shouldLockRuntime is true)
        return workletRunner_.callUnsafe(
            inputJsArray,
            outputJsArray,
            jsi::Value(rt, static_cast<int>(framesToProcess)),
            jsi::Value(rt, this->context_->getCurrentTime()));
      });

  // Copy processed output data back to the processing bus or zero on failure
  for (size_t ch = 0; ch < channelCount; ch++) {
    auto channelData = processingBus->getChannel(ch)->getData();

    if (result.has_value()) {
      // Copy processed output data
      std::memcpy(
          /* dest */ reinterpret_cast<uint8_t *>(channelData),
          /* src */ outputBuffsHandles_[ch]->data(),
          /* size */ framesToProcess * sizeof(float));
    } else {
      // Zero the output on worklet execution failure
      std::memset(channelData, 0, framesToProcess * sizeof(float));
    }
  }

  return processingBus;
}

} // namespace audioapi
