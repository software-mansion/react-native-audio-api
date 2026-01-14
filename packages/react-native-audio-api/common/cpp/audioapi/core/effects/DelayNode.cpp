#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <memory>

namespace audioapi {

DelayNode::DelayNode(std::shared_ptr<BaseAudioContext> context, float maxDelayTime)
    : AudioNode(context),
      delayTimeParam_(std::make_shared<AudioParam>(0, 0, maxDelayTime, context)),
      delayBuffer_(
          std::make_shared<AudioBus>(
              static_cast<size_t>(
                  maxDelayTime * context->getSampleRate() +
                  1), // +1 to enable delayTime equal to maxDelayTime
              channelCount_,
              context->getSampleRate())) {
  requiresTailProcessing_ = true;
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> DelayNode::getDelayTimeParam() const {
  return delayTimeParam_;
}

void DelayNode::onInputDisabled() {
  numberOfEnabledInputNodes_ -= 1;
  if (isEnabled() && numberOfEnabledInputNodes_ == 0) {
    signalledToStop_ = true;
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      remainingFrames_ = delayTimeParam_->getValue() * context->getSampleRate();
    } else {
      remainingFrames_ = 0;
    }
  }
}

void DelayNode::delayBufferOperation(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess,
    size_t &operationStartingIndex,
    DelayNode::BufferAction action) {
  size_t processingBusStartIndex = 0;

  // handle buffer wrap around
  if (operationStartingIndex + framesToProcess > delayBuffer_->getSize()) {
    int framesToEnd = operationStartingIndex + framesToProcess - delayBuffer_->getSize();

    if (action == BufferAction::WRITE) {
      delayBuffer_->sum(
          processingBus.get(), processingBusStartIndex, operationStartingIndex, framesToEnd);
    } else { // READ
      processingBus->sum(
          delayBuffer_.get(), operationStartingIndex, processingBusStartIndex, framesToEnd);
    }

    operationStartingIndex = 0;
    processingBusStartIndex += framesToEnd;
    framesToProcess -= framesToEnd;
  }

  if (action == BufferAction::WRITE) {
    delayBuffer_->sum(
        processingBus.get(), processingBusStartIndex, operationStartingIndex, framesToProcess);
    processingBus->zero();
  } else { // READ
    processingBus->sum(
        delayBuffer_.get(), operationStartingIndex, processingBusStartIndex, framesToProcess);
    delayBuffer_->zero(operationStartingIndex, framesToProcess);
  }

  operationStartingIndex += framesToProcess;
}

// delay buffer always has channelCount_ channels
// processing is split into two parts
// 1. writing to delay buffer (mixing if needed) from processing bus
// 2. reading from delay buffer to processing bus (mixing if needed) with delay
std::shared_ptr<AudioBus> DelayNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  // handling tail processing
  if (signalledToStop_) {
    if (remainingFrames_ <= 0) {
      disable();
      signalledToStop_ = false;
      return processingBus;
    }

    delayBufferOperation(processingBus, framesToProcess, readIndex_, DelayNode::BufferAction::READ);
    remainingFrames_ -= framesToProcess;
    return processingBus;
  }

  // normal processing
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr)
    return processingBus;
  auto delayTime = delayTimeParam_->processKRateParam(framesToProcess, context->getCurrentTime());
  size_t writeIndex = static_cast<size_t>(readIndex_ + delayTime * context->getSampleRate()) %
      delayBuffer_->getSize();
  delayBufferOperation(processingBus, framesToProcess, writeIndex, DelayNode::BufferAction::WRITE);
  delayBufferOperation(processingBus, framesToProcess, readIndex_, DelayNode::BufferAction::READ);

  return processingBus;
}

} // namespace audioapi
