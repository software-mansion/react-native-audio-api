#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <memory>

namespace audioapi {

DelayNode::DelayNode(const std::shared_ptr<BaseAudioContext> &context, const DelayOptions &options)
    : AudioNode(context, options),
      delayTimeParam_(
          std::make_shared<AudioParam>(options.delayTime, 0, options.maxDelayTime, context)),
      delayBuffer_(
          std::make_shared<AudioBuffer>(
              static_cast<size_t>(
                  options.maxDelayTime * context->getSampleRate() +
                  1), // +1 to enable delayTime equal to maxDelayTime
              channelCount_,
              context->getSampleRate())) {
  isInitialized_.store(true, std::memory_order_release);
}

std::shared_ptr<AudioParam> DelayNode::getDelayTimeParam() const {
  return delayTimeParam_;
}

void DelayNode::onInputDisabled() {
  numberOfEnabledInputNodes_ -= 1;
  if (isEnabled() && numberOfEnabledInputNodes_ == 0) {
    signalledToStop_ = true;
    remainingFrames_ = delayTimeParam_->getValue() * getContextSampleRate();
  }
}

void DelayNode::delayBufferOperation(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess,
    size_t &operationStartingIndex,
    DelayNode::BufferAction action) {
  size_t processingBufferStartIndex = 0;

  // handle buffer wrap around
  if (operationStartingIndex + framesToProcess > delayBuffer_->getSize()) {
    int framesToEnd = operationStartingIndex + framesToProcess - delayBuffer_->getSize();

    if (action == BufferAction::WRITE) {
      delayBuffer_->sum(
          *processingBuffer, processingBufferStartIndex, operationStartingIndex, framesToEnd);
    } else { // READ
      processingBuffer->sum(
          *delayBuffer_, operationStartingIndex, processingBufferStartIndex, framesToEnd);
    }

    operationStartingIndex = 0;
    processingBufferStartIndex += framesToEnd;
    framesToProcess -= framesToEnd;
  }

  if (action == BufferAction::WRITE) {
    delayBuffer_->sum(
        *processingBuffer, processingBufferStartIndex, operationStartingIndex, framesToProcess);
    processingBuffer->zero();
  } else { // READ
    processingBuffer->sum(
        *delayBuffer_, operationStartingIndex, processingBufferStartIndex, framesToProcess);
    delayBuffer_->zero(operationStartingIndex, framesToProcess);
  }

  operationStartingIndex += framesToProcess;
}

// delay buffer always has channelCount_ channels
// processing is split into two parts
// 1. writing to delay buffer (mixing if needed) from processing buffer
// 2. reading from delay buffer to processing buffer (mixing if needed) with delay
std::shared_ptr<DSPAudioBuffer> DelayNode::processNode(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  // handling tail processing
  if (signalledToStop_) {
    if (remainingFrames_ <= 0) {
      disable();
      signalledToStop_ = false;
      return processingBuffer;
    }

    delayBufferOperation(
        processingBuffer, framesToProcess, readIndex_, DelayNode::BufferAction::READ);
    remainingFrames_ -= framesToProcess;
    return processingBuffer;
  }

  // normal processing
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }

  auto delayTime = delayTimeParam_->processKRateParam(framesToProcess, context->getCurrentTime());
  size_t writeIndex = static_cast<size_t>(readIndex_ + delayTime * context->getSampleRate()) %
      delayBuffer_->getSize();
  delayBufferOperation(
      processingBuffer, framesToProcess, writeIndex, DelayNode::BufferAction::WRITE);
  delayBufferOperation(
      processingBuffer, framesToProcess, readIndex_, DelayNode::BufferAction::READ);

  return processingBuffer;
}

} // namespace audioapi
