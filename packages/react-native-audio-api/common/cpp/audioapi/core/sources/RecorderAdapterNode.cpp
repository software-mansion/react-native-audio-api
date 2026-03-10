
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

namespace audioapi {

RecorderAdapterNode::RecorderAdapterNode(const std::shared_ptr<BaseAudioContext> &context)
    : AudioNode(context, AudioScheduledSourceNodeOptions()) {
  // It should be marked as initialized only after it is connected to the
  // recorder. Internal buffer size is based on the recorder's buffer length.
  isInitialized_.store(false, std::memory_order_release);
}

void RecorderAdapterNode::init(size_t bufferSize, int channelCount, float sampleRate) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (isInitialized_.load(std::memory_order_acquire) || context == nullptr) {
    return;
  }

  channelCount_ = channelCount;

  buff_.resize(channelCount_);

  for (size_t i = 0; i < channelCount_; ++i) {
    buff_[i] = std::make_shared<CircularOverflowableAudioArray>(bufferSize);
  }

  float contextSampleRate = context->getSampleRate();
  needsResampling_ = static_cast<int>(sampleRate) != static_cast<int>(contextSampleRate);

  adapterOutputBuffer_ =
      std::make_shared<AudioBuffer>(RENDER_QUANTUM_SIZE, channelCount_, contextSampleRate);

  if (needsResampling_) {
    inputChunkSize_ =
        static_cast<size_t>(std::ceil(RENDER_QUANTUM_SIZE * sampleRate / contextSampleRate)) + 4;

    resampler_ = std::make_unique<r8b::MultiChannelResampler>(
        sampleRate, contextSampleRate, channelCount_, static_cast<int>(inputChunkSize_));

    const int maxOutLen = resampler_->getMaxOutLen();

    resamplerInputBuffer_ = AudioBuffer(inputChunkSize_, channelCount_, sampleRate);
    resamplerOutputBuffer_ =
        AudioBuffer(static_cast<size_t>(maxOutLen), channelCount_, contextSampleRate);
    overflowBuffer_ = AudioBuffer(2 * maxOutLen, channelCount_, contextSampleRate);
    overflowSize_ = 0;
  }

  isInitialized_.store(true, std::memory_order_release);
}

void RecorderAdapterNode::cleanup() {
  needsResampling_ = false;
  buff_.clear();
  resampler_.reset();
  overflowSize_ = 0;

  isInitialized_.store(false, std::memory_order_release);
}

std::shared_ptr<AudioBuffer> RecorderAdapterNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (!isInitialized_.load(std::memory_order_acquire)) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (needsResampling_) {
    processResampled(framesToProcess);
  } else {
    readFrames(*adapterOutputBuffer_, framesToProcess);
  }

  processingBuffer->sum(*adapterOutputBuffer_, ChannelInterpretation::SPEAKERS);
  return processingBuffer;
}

void RecorderAdapterNode::processResampled(int framesToProcess) {
  adapterOutputBuffer_->zero();

  size_t outputWritten = 0;
  const size_t needed = static_cast<size_t>(framesToProcess);

  // Drain leftover resampled samples from the previous call
  if (overflowSize_ > 0) {
    const size_t toCopy = std::min(overflowSize_, needed);

    adapterOutputBuffer_->copy(overflowBuffer_, 0, 0, toCopy);
    outputWritten = toCopy;

    if (toCopy < overflowSize_) {
      const size_t remaining = overflowSize_ - toCopy;
      for (int ch = 0; ch < channelCount_; ++ch) {
        overflowBuffer_[ch].copyWithin(toCopy, 0, remaining);
      }
    }
    overflowSize_ -= toCopy;
  }

  // Feed the resampler until we have enough output frames
  while (outputWritten < needed) {
    readFrames(resamplerInputBuffer_, inputChunkSize_);

    const int outLen = resampler_->process(
        resamplerInputBuffer_, static_cast<int>(inputChunkSize_), resamplerOutputBuffer_);

    const size_t remaining = needed - outputWritten;
    const size_t toCopy = std::min(static_cast<size_t>(outLen), remaining);

    // Write resampled frames into the output buffer
    adapterOutputBuffer_->copy(resamplerOutputBuffer_, 0, outputWritten, toCopy);
    outputWritten += toCopy;

    // Stash excess for the next processNode call
    const int excess = outLen - static_cast<int>(toCopy);
    if (excess > 0) {
      overflowBuffer_.copy(
          resamplerOutputBuffer_, toCopy, overflowSize_, static_cast<size_t>(excess));
      overflowSize_ += static_cast<size_t>(excess);
    }
  }
}

void RecorderAdapterNode::readFrames(AudioBuffer &target, const size_t framesToRead) {
  target.zero();

  for (size_t channel = 0; channel < channelCount_; ++channel) {
    buff_[channel]->read(*target.getChannel(channel), framesToRead);
  }
}

} // namespace audioapi
