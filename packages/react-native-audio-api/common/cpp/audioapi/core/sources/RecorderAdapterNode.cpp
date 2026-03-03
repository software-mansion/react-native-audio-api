
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>

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
  isInitialized_ = false;
}

void RecorderAdapterNode::init(size_t bufferSize, int channelCount, float sampleRate) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (isInitialized_ || context == nullptr) {
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
        sampleRate, contextSampleRate, channelCount_, inputChunkSize_);

    resamplerInputBuffer_ =
        std::make_shared<AudioBuffer>(inputChunkSize_, channelCount_, sampleRate);

    overflowBuffers_.resize(channelCount_);
    overflowSize_ = 0;
  }

  isInitialized_ = true;
}

void RecorderAdapterNode::cleanup() {
  isInitialized_ = false;
  needsResampling_ = false;
  buff_.clear();
  adapterOutputBuffer_.reset();
  resamplerInputBuffer_.reset();
  resampler_.reset();
  overflowBuffers_.clear();
  overflowSize_ = 0;
}

std::shared_ptr<AudioBuffer> RecorderAdapterNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (!isInitialized_) {
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
  size_t needed = static_cast<size_t>(framesToProcess);

  // Drain leftover resampled samples from the previous call
  if (overflowSize_ > 0) {
    size_t toCopy = std::min(overflowSize_, needed);

    for (int ch = 0; ch < channelCount_; ++ch) {
      adapterOutputBuffer_->getChannel(ch)->copy(overflowBuffers_[ch].data(), 0, 0, toCopy);
    }
    outputWritten = toCopy;

    if (toCopy < overflowSize_) {
      for (int ch = 0; ch < channelCount_; ++ch) {
        std::memmove(
            overflowBuffers_[ch].data(),
            overflowBuffers_[ch].data() + toCopy,
            (overflowSize_ - toCopy) * sizeof(float));
      }
    }
    overflowSize_ -= toCopy;
  }

  // Feed the resampler until we have enough output frames
  while (outputWritten < needed) {
    readFrames(*resamplerInputBuffer_, inputChunkSize_);

    std::vector<float *> inputPtrs(channelCount_);
    std::vector<float *> outputPtrs(channelCount_, nullptr);

    for (int ch = 0; ch < channelCount_; ++ch) {
      inputPtrs[ch] = resamplerInputBuffer_->getChannel(ch)->begin();
    }

    int outLen = resampler_->process(inputPtrs, static_cast<int>(inputChunkSize_), outputPtrs);

    if (outLen <= 0) {
      continue;
    }

    size_t remaining = needed - outputWritten;
    size_t toCopy = std::min(static_cast<size_t>(outLen), remaining);

    // Write resampled frames into the output buffer
    for (int ch = 0; ch < channelCount_; ++ch) {
      adapterOutputBuffer_->getChannel(ch)->copy(outputPtrs[ch], 0, outputWritten, toCopy);
    }
    outputWritten += toCopy;

    // Stash excess for the next processNode call
    int excess = outLen - toCopy;
    if (excess > 0) {
      for (int ch = 0; ch < channelCount_; ++ch) {
        overflowBuffers_[ch].resize(overflowSize_ + excess);
        std::memcpy(
            overflowBuffers_[ch].data() + overflowSize_,
            outputPtrs[ch] + toCopy,
            excess * sizeof(float));
      }
      overflowSize_ += excess;
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
