#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

DelayNode::DelayNode(BaseAudioContext *context, float maxDelayTime)
    : AudioNode(context) {
  delayTimeParam_ = std::make_shared<AudioParam>(0, 0, maxDelayTime, context);
  delayBuffer_ = std::make_shared<AudioBus>(
      static_cast<size_t>(
          maxDelayTime * context->getSampleRate() +
          1), // +1 to enable delayTime equal to maxDelayTime
      2,
      context->getSampleRate());
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> DelayNode::getDelayTimeParam() const {
  return delayTimeParam_;
}

void DelayNode::onInputDisabled() {
  numberOfEnabledInputNodes_ -= 1;
  if (isEnabled() && numberOfEnabledInputNodes_ == 0) {
    signalledToStop_ = true;
    remainingFrames_ = delayTimeParam_->getValue() * context_->getSampleRate();
  }
}

// delay buffer always has 2 channels, mix if needed
std::shared_ptr<AudioBus> DelayNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  if (signalledToStop_) {
    if (remainingFrames_ > 0) {
      if (readIndex_ + framesToProcess >= delayBuffer_->getSize()) {
        size_t framesToEnd = delayBuffer_->getSize() - readIndex_;
        processingBus->sum(delayBuffer_.get(), readIndex_, 0, framesToEnd);
        delayBuffer_->zero(readIndex_, framesToEnd);
        readIndex_ = 0;
        framesToProcess -= framesToEnd;
        remainingFrames_ -= framesToEnd;
      }
      processingBus->sum(delayBuffer_.get(), readIndex_, 0, framesToProcess);
      delayBuffer_->zero(readIndex_, framesToProcess);
      remainingFrames_ -= framesToProcess;
      readIndex_ += framesToProcess;
    } else {
      disable();
      signalledToStop_ = false;
    }
    return processingBus;
  }
  auto delayTime = delayTimeParam_->processKRateParam(
      framesToProcess, context_->getCurrentTime());
  size_t processingBusStartIndex = 0;
  size_t writeIndex =
      static_cast<size_t>(readIndex_ + delayTime * context_->getSampleRate()) %
      delayBuffer_->getSize();
  int framesToWrite = framesToProcess;
  if (writeIndex + framesToWrite >= delayBuffer_->getSize()) {
    int framesToCopy = writeIndex + framesToWrite - delayBuffer_->getSize();
    delayBuffer_->sum(
        processingBus.get(), processingBusStartIndex, writeIndex, framesToCopy);
    writeIndex = 0;
    processingBusStartIndex += framesToCopy;
    framesToWrite -= framesToCopy;
  }
  delayBuffer_->sum(
      processingBus.get(), processingBusStartIndex, writeIndex, framesToWrite);
  processingBus->zero();
  if (readIndex_ + framesToProcess >= delayBuffer_->getSize()) {
    size_t framesToEnd = delayBuffer_->getSize() - readIndex_;
    processingBus->sum(delayBuffer_.get(), readIndex_, 0, framesToEnd);
    readIndex_ = 0;
    framesToProcess -= framesToEnd;
    delayBuffer_->zero(readIndex_, framesToEnd);
  }
  processingBus->sum(delayBuffer_.get(), readIndex_, 0, framesToProcess);
  delayBuffer_->zero(readIndex_, framesToProcess);
  readIndex_ += framesToProcess;
  return processingBus;
}

} // namespace audioapi
