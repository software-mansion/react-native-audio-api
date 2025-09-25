#include <audioapi/core/effects/WorkletNode.h>

namespace audioapi {

WorkletNode::WorkletNode(
    BaseAudioContext *context,
    std::shared_ptr<worklets::SerializableWorklet> &worklet,
    size_t bufferLength,
    size_t inputChannelCount)
    : AudioNode(context),
      buffRealLength_(bufferLength * sizeof(float)),
      bufferLength_(bufferLength),
      workletRunner_(context->workletRunner_),
      shareableWorklet_(worklet),
      inputChannelCount_(inputChannelCount),
      curBuffIndex_(0) {
  buffs_.reserve(inputChannelCount_);
  for (size_t i = 0; i < inputChannelCount_; ++i) {
    buffs_.emplace_back(new uint8_t[buffRealLength_]);
  }
  isInitialized_ = true;
}

WorkletNode::~WorkletNode() {
  for (auto &buff : buffs_) {
    delete[] buff;
  }
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

    for (size_t ch = 0; ch < channelCount_; ch++) {
      /// here we copy
      /// to   uint8_t* [curBuffIndex_, curBuffIndex_ + shouldProcess]
      /// from float* [processed, processed + shouldProcess]
      /// so as the we need to copy shouldProcess * sizeof(float) bytes
      auto channelData = processingBus->getChannel(ch)->getData();
      std::memcpy(
          /* dest */ buffs_[ch] + curBuffIndex_ * sizeof(float),
          /* src */ reinterpret_cast<const uint8_t *>(channelData + processed),
          /* size */ shouldProcess * sizeof(float));
    }
    processed += shouldProcess;
    curBuffIndex_ += shouldProcess;

    /// If we filled the entire buffer, we need to execute the worklet
    if (curBuffIndex_ == bufferLength_) {
      // Reset buffer index, channel buffers and execute worklet
      curBuffIndex_ = 0;
      workletRunner_->executeOnRuntimeGuardedSync(
          [this, channelCount_](jsi::Runtime &uiRuntimeRaw) {
            /// Arguments preparation
            auto jsArray = jsi::Array(uiRuntimeRaw, channelCount_);
            for (size_t ch = 0; ch < channelCount_; ch++) {
              uint8_t *buffPtr = buffs_[ch];
              buffs_[ch] = new uint8_t[buffRealLength_];
              auto sharedAudioArray =
                  std::make_shared<AudioArrayBuffer>(buffPtr, buffRealLength_);
              auto arrayBuffer =
                  jsi::ArrayBuffer(uiRuntimeRaw, std::move(sharedAudioArray));
              jsArray.setValueAtIndex(uiRuntimeRaw, ch, std::move(arrayBuffer));
            }
            jsArray.setExternalMemoryPressure(
                uiRuntimeRaw, channelCount_ * buffRealLength_);

            workletRunner_->executeWorklet(
                shareableWorklet_,
                std::move(jsArray),
                jsi::Value(uiRuntimeRaw, static_cast<int>(channelCount_)));
            return jsi::Value::undefined();
          });
    }
  }
}

} // namespace audioapi
