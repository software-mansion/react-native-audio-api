#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace audioapi {

namespace {

/// Frames the ring must hold: the current quantum, plus the longest delay
/// (one quantum in a feedback loop even when `maxDelayTime` is shorter), plus
/// the neighbour that interpolating a fractional delay reads.
size_t delayBufferFrames(float maxDelayTime, float sampleRate) {
  const auto maxDelayFrames = static_cast<size_t>(std::ceil(maxDelayTime * sampleRate));
  return std::max(maxDelayFrames, static_cast<size_t>(RENDER_QUANTUM_SIZE)) + RENDER_QUANTUM_SIZE +
      1;
}

/// `delayTime` is a float, so a whole number of frames comes back off by up
/// to its rounding error; snap that away so such a delay does not interpolate.
double delayTimeToFrames(float delayTime, float sampleRate) {
  const double frames = static_cast<double>(delayTime) * sampleRate;
  const double wholeFrames = std::round(frames);
  const double floatRoundingError = wholeFrames * std::numeric_limits<float>::epsilon();
  return std::fabs(frames - wholeFrames) <= floatRoundingError ? wholeFrames : frames;
}

} // namespace

DelayNode::DelayNode(const std::shared_ptr<BaseAudioContext> &context, const DelayOptions &options)
    : AudioNode(context, options),
      delayTimeParam_(
          std::make_shared<AudioParam>(options.delayTime, 0, options.maxDelayTime, context)),
      delayBuffer_(
          std::make_shared<AudioBuffer>(
              delayBufferFrames(options.maxDelayTime, context->getSampleRate()),
              channelCount_,
              context->getSampleRate())),
      pendingDelayBuffer_(delayBuffer_) {}

std::shared_ptr<AudioParam> DelayNode::getDelayTimeParam() const {
  return delayTimeParam_;
}

size_t DelayNode::getDelayBufferSize() const {
  return delayBuffer_->getSize();
}

std::shared_ptr<AudioBuffer> DelayNode::buildNegotiatedStorage(size_t numberOfChannels) {
  pendingDelayBuffer_ = pendingDelayBuffer_->withNumberOfChannels(numberOfChannels);
  return pendingDelayBuffer_;
}

std::shared_ptr<AudioBuffer> DelayNode::exchangeNegotiatedStorage(
    std::shared_ptr<AudioBuffer> storage) {
  return std::exchange(delayBuffer_, std::move(storage));
}

void DelayNode::processNode(int framesToProcess) {
  // The inputs of this quantum are mixed into audioBuffer_. Write them
  // first, so a delay shorter than the quantum already reads them below.
  const DSPAudioBuffer *mixedInputs = audioBuffer_.get();
  writeQuantum(std::span(&mixedInputs, 1), framesToProcess);
  readQuantum(framesToProcess, 0.0);
  advanceWriteIndex(framesToProcess);
}

void DelayNode::processReader(int framesToProcess) {
  // The writer has not stored this quantum's inputs yet, so the delay can
  // reach back no further than the previous quantum.
  readQuantum(framesToProcess, RENDER_QUANTUM_SIZE);
}

void DelayNode::processWriter(
    const std::vector<const DSPAudioBuffer *> &inputs,
    int framesToProcess) {
  updateTailStateForQuantum(inputs, framesToProcess);
  writeQuantum(inputs, framesToProcess);
  advanceWriteIndex(framesToProcess);
}

int DelayNode::computeTailFrames() const {
  return static_cast<int>(delayTimeParam_->getMaxValue() * delayBuffer_->getSampleRate());
}

void DelayNode::writeQuantum(std::span<const DSPAudioBuffer *const> inputs, int framesToProcess) {
  const auto frames = static_cast<size_t>(framesToProcess);
  const size_t framesBeforeWrap = std::min(frames, delayBuffer_->getSize() - writeIndex_);
  const size_t framesAfterWrap = frames - framesBeforeWrap;

  delayBuffer_->zero(writeIndex_, framesBeforeWrap);
  delayBuffer_->zero(0, framesAfterWrap);
  for (const DSPAudioBuffer *input : inputs) {
    delayBuffer_->sum(*input, 0, writeIndex_, framesBeforeWrap, channelInterpretation_);
    if (framesAfterWrap > 0) {
      delayBuffer_->sum(*input, framesBeforeWrap, 0, framesAfterWrap, channelInterpretation_);
    }
  }
}

void DelayNode::readQuantum(int framesToProcess, double minDelayFrames) {
  audioBuffer_->zero();
  auto context = context_.lock();
  if (context == nullptr) {
    return;
  }

  const float sampleRate = context->getSampleRate();
  const auto delayTimes =
      delayTimeParam_->processARateParam(framesToProcess, context->getCurrentTime())
          ->getChannel(0)
          ->span();
  const double maxDelayFrames =
      std::max(static_cast<double>(delayTimeParam_->getMaxValue()) * sampleRate, minDelayFrames);

  const size_t ringSize = delayBuffer_->getSize();

  for (int i = 0; i < framesToProcess; ++i) {
    // delayTime may fall between frames, so the read position can be fractional.
    const double delayFrames =
        std::clamp(delayTimeToFrames(delayTimes[i], sampleRate), minDelayFrames, maxDelayFrames);
    // ringSize exceeds every delay, so adding it keeps the position positive.
    const double position = static_cast<double>(writeIndex_ + i + ringSize) - delayFrames;
    const double wholeFrames = std::floor(position);
    const auto fraction = static_cast<float>(position - wholeFrames);
    const size_t earlier = static_cast<size_t>(wholeFrames) % ringSize;
    const size_t later = (earlier + 1) % ringSize;

    for (size_t channel = 0; channel < delayBuffer_->getNumberOfChannels(); ++channel) {
      const auto &ring = (*delayBuffer_)[channel];
      float sample = ring[earlier];
      if (fraction > 0.0f) {
        // linear interpolation
        sample += fraction * (ring[later] - sample);
      }
      (*audioBuffer_)[channel][i] = sample;
    }
  }
}

void DelayNode::advanceWriteIndex(int framesToProcess) {
  writeIndex_ = (writeIndex_ + static_cast<size_t>(framesToProcess)) % delayBuffer_->getSize();
}

} // namespace audioapi
