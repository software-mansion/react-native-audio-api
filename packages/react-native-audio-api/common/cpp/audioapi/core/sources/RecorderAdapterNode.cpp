#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/CurrentRenderScope.h>
#include <audioapi/utils/AudioArray.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace audioapi {

RecorderAdapterConsumer::RecorderAdapterConsumer(float contextSampleRate)
    : contextSampleRate_(contextSampleRate) {}

Result<NoneType, std::string> RecorderAdapterConsumer::prepare(const StreamFormat &streamFormat) {
  const float sampleRate = streamFormat.layout.sampleRate;
  const int channelCount = streamFormat.layout.channelCount;
  outputChannelNumber_.store(channelCount, std::memory_order_release);

  buff_.resize(channelCount);

  for (int i = 0; i < channelCount; ++i) {
    buff_[i] = std::make_shared<CircularOverflowableAudioArray>(streamFormat.maxFramesPerBuffer);
  }

  needsResampling_ = static_cast<int>(sampleRate) != static_cast<int>(contextSampleRate_);

  adapterOutputBuffer_ =
      std::make_shared<AudioBuffer>(RENDER_QUANTUM_SIZE, channelCount, contextSampleRate_);

  if (needsResampling_) {
    inputChunkSize_ =
        static_cast<size_t>(std::ceil(RENDER_QUANTUM_SIZE * sampleRate / contextSampleRate_)) + 4;

    resampler_ = std::make_unique<r8b::MultiChannelResampler>(
        sampleRate, contextSampleRate_, channelCount, static_cast<int>(inputChunkSize_));

    const int maxOutLen = resampler_->getMaxOutLen();

    resamplerInputBuffer_ = AudioBuffer(inputChunkSize_, channelCount, sampleRate);
    resamplerOutputBuffer_ =
        AudioBuffer(static_cast<size_t>(maxOutLen), channelCount, contextSampleRate_);
    overflowBuffer_ = AudioBuffer(2 * maxOutLen, channelCount, contextSampleRate_);
    overflowSize_ = 0;
  }

  isInitialized_.store(true, std::memory_order_release);
  return Ok(None);
}

void RecorderAdapterConsumer::close() {
  isInitialized_.store(false, std::memory_order_release);
  waitForProcessQuiescence();

  needsResampling_ = false;
  buff_.clear();
  resampler_.reset();
  overflowSize_ = 0;
}

Result<NoneType, std::string> RecorderAdapterConsumer::consume(
    const AudioBuffer &frames,
    size_t numFrames) {
  for (size_t channel = 0; channel < buff_.size(); ++channel) {
    buff_[channel]->write(frames.getChannel(channel)->begin(), numFrames);
  }
  return Ok(None);
}

void RecorderAdapterConsumer::waitForProcessQuiescence() const {
  while (currentProcesses_.load(std::memory_order_acquire) != 0) {
    std::this_thread::yield();
  }
}

void RecorderAdapterConsumer::render(DSPAudioBuffer &output, int framesToProcess) {
  const CurrentRenderScope processScope(currentProcesses_);

  if (!isInitialized_.load(std::memory_order_acquire)) {
    output.zero();
    return;
  }

  if (needsResampling_) {
    processResampled(framesToProcess);
  } else {
    readFrames(*adapterOutputBuffer_, framesToProcess);
  }

  output.sum(*adapterOutputBuffer_, ChannelInterpretation::SPEAKERS);
}

void RecorderAdapterConsumer::processResampled(int framesToProcess) {
  adapterOutputBuffer_->zero();

  size_t outputWritten = 0;
  const auto needed = static_cast<size_t>(framesToProcess);

  // Drain leftover resampled samples from the previous call
  if (overflowSize_ > 0) {
    const size_t toCopy = std::min(overflowSize_, needed);

    adapterOutputBuffer_->copy(overflowBuffer_, 0, 0, toCopy);
    outputWritten = toCopy;

    if (toCopy < overflowSize_) {
      const size_t remaining = overflowSize_ - toCopy;
      for (size_t ch = 0; ch < buff_.size(); ++ch) {
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

    // Stash excess for the next render call
    const int excess = outLen - static_cast<int>(toCopy);
    if (excess > 0) {
      overflowBuffer_.copy(
          resamplerOutputBuffer_, toCopy, overflowSize_, static_cast<size_t>(excess));
      overflowSize_ += static_cast<size_t>(excess);
    }
  }
}

void RecorderAdapterConsumer::readFrames(AudioBuffer &target, const size_t framesToRead) {
  target.zero();

  for (size_t channel = 0; channel < buff_.size(); ++channel) {
    buff_[channel]->read(*target.getChannel(channel), framesToRead);
  }
}

RecorderAdapterNode::RecorderAdapterNode(const std::shared_ptr<BaseAudioContext> &context)
    : AudioNode(context, AudioScheduledSourceNodeOptions()),
      consumer_(std::make_shared<RecorderAdapterConsumer>(context->getSampleRate())) {}

std::optional<size_t> RecorderAdapterNode::getOutputChannelNumber() const {
  return consumer_->outputChannelCount();
}

void RecorderAdapterNode::processNode(int framesToProcess) {
  consumer_->render(*audioBuffer_, framesToProcess);
}

} // namespace audioapi
