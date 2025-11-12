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
      channelCount_,
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

std::shared_ptr<AudioBus> DelayNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  // Mismatched channel count, mix delay buffer to match processing bus
  if (processingBus->getNumberOfChannels() !=
      delayBuffer_->getNumberOfChannels()) {
    AudioBus mixedDelayBuffer(
        delayBuffer_->getSize(),
        processingBus->getNumberOfChannels(),
        context_->getSampleRate());
    mixedDelayBuffer.zero();
    mixedDelayBuffer.sum(delayBuffer_.get());
    delayBuffer_ = std::make_shared<AudioBus>(mixedDelayBuffer);
  }
  if (signalledToStop_) {
    if (remainingFrames_ > 0) {
      for (int frame = 0; frame < std::min(framesToProcess, remainingFrames_);
           ++frame) {
        for (int channel = 0; channel < processingBus->getNumberOfChannels();
             ++channel) {
          processingBus->getChannel(channel)->getData()[frame] =
              delayBuffer_->getChannel(channel)->getData()[readIndex_];
        }
        readIndex_ = (readIndex_ + 1) % delayBuffer_->getSize();
      }
      remainingFrames_ -= framesToProcess;
    } else {
      disable();
      signalledToStop_ = false;
    }
    return processingBus;
  }
  double time = context_->getCurrentTime();
  auto delayTimeParamValues =
      delayTimeParam_->processARateParam(framesToProcess, time);
  auto sampleRate = context_->getSampleRate();
  for (int frame = 0; frame < framesToProcess; ++frame) {
    float delayTime = (*delayTimeParamValues->getChannel(0))[frame];
    size_t delaySamples = static_cast<size_t>(delayTime * sampleRate);
    size_t writeIndex = (readIndex_ + delaySamples) % delayBuffer_->getSize();

    for (int channel = 0; channel < processingBus->getNumberOfChannels();
         ++channel) {
      // Write the current input sample into the delay buffer
      float inputSample = processingBus->getChannel(channel)->getData()[frame];
      delayBuffer_->getChannel(channel)->getData()[writeIndex] = inputSample;

      // Output the delayed sample
      float delayedSample =
          delayBuffer_->getChannel(channel)->getData()[readIndex_];
      processingBus->getChannel(channel)->getData()[frame] = delayedSample;
    }

    readIndex_ = (readIndex_ + 1) % delayBuffer_->getSize();
  }
  return processingBus;
}

} // namespace audioapi
