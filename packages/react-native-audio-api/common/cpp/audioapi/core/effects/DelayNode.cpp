#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>

#include <algorithm>
#include <memory>
#include <vector>

namespace audioapi {

DelayNode::DelayNode(const std::shared_ptr<BaseAudioContext> &context, const DelayOptions &options)
    : AudioNode(context, options),
      delayTimeParam_(
          std::make_shared<AudioParam>(options.delayTime, 0, options.maxDelayTime, context)),
      delayBuffer_(
          std::make_shared<AudioBuffer>(
              std::max(
                  static_cast<size_t>(options.maxDelayTime * context->getSampleRate()),
                  static_cast<size_t>(RENDER_QUANTUM_SIZE)) +
                  RENDER_QUANTUM_SIZE,
              channelCount_,
              context->getSampleRate())) {}

std::shared_ptr<AudioParam> DelayNode::getDelayTimeParam() const {
  return delayTimeParam_;
}

size_t DelayNode::getDelayBufferSize() const {
  return delayBuffer_->getSize();
}

void DelayNode::processNode(int framesToProcess) {
  // The inputs of this quantum are mixed into audioBuffer_. Write them
  // first, so a delay shorter than the quantum already reads them below.
  writeToDelayBuffer(*audioBuffer_, readIndex_ + currentDelayFrames(), framesToProcess);
  processReader(framesToProcess);
}

void DelayNode::processReader(int framesToProcess) {
  audioBuffer_->zero();
  readFromDelayBuffer(framesToProcess);
}

void DelayNode::processWriter(
    const std::vector<const DSPAudioBuffer *> &inputs,
    int framesToProcess) {
  updateTailStateForQuantum(inputs, framesToProcess);

  // The reader already rendered this quantum, so what is written now can come
  // out in the next quantum at the earliest.
  const size_t delayFrames =
      std::max(currentDelayFrames(), static_cast<size_t>(RENDER_QUANTUM_SIZE));
  // The reader moved the read head past this quantum.
  const size_t quantumStart =
      readIndex_ + delayBuffer_->getSize() - static_cast<size_t>(framesToProcess);
  for (const DSPAudioBuffer *input : inputs) {
    writeToDelayBuffer(*input, quantumStart + delayFrames, framesToProcess);
  }
}

int DelayNode::computeTailFrames() const {
  return static_cast<int>(delayTimeParam_->getMaxValue() * delayBuffer_->getSampleRate());
}

size_t DelayNode::currentDelayFrames() const {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    return 0;
  }
  const float delayTime = delayTimeParam_->processKRateParam(context->getCurrentTime());
  return static_cast<size_t>(delayTime * context->getSampleRate());
}

void DelayNode::writeToDelayBuffer(const DSPAudioBuffer &source, size_t ringIndex, int frames) {
  const size_t ringSize = delayBuffer_->getSize();
  ringIndex %= ringSize;
  const size_t framesBeforeWrap = std::min(static_cast<size_t>(frames), ringSize - ringIndex);
  const size_t framesAfterWrap = static_cast<size_t>(frames) - framesBeforeWrap;

  delayBuffer_->sum(source, 0, ringIndex, framesBeforeWrap, channelInterpretation_);
  if (framesAfterWrap > 0) {
    delayBuffer_->sum(source, framesBeforeWrap, 0, framesAfterWrap, channelInterpretation_);
  }
}

void DelayNode::readFromDelayBuffer(int frames) {
  const size_t ringSize = delayBuffer_->getSize();
  const size_t framesBeforeWrap = std::min(static_cast<size_t>(frames), ringSize - readIndex_);
  const size_t framesAfterWrap = static_cast<size_t>(frames) - framesBeforeWrap;

  audioBuffer_->sum(*delayBuffer_, readIndex_, 0, framesBeforeWrap, channelInterpretation_);
  delayBuffer_->zero(readIndex_, framesBeforeWrap);
  if (framesAfterWrap > 0) {
    audioBuffer_->sum(*delayBuffer_, 0, framesBeforeWrap, framesAfterWrap, channelInterpretation_);
    delayBuffer_->zero(0, framesAfterWrap);
  }

  readIndex_ = (readIndex_ + static_cast<size_t>(frames)) % ringSize;
}

} // namespace audioapi
