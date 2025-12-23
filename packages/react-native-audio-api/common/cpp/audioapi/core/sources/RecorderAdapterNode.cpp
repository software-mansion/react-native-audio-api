
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <memory>
#include <type_traits>

namespace audioapi {

RecorderAdapterNode::RecorderAdapterNode(std::shared_ptr<BaseAudioContext> context)
    : AudioNode(context) {
  // It should be marked as initialized only after it is connected to the
  // recorder. Internall buffer size is based on the recorder's buffer length.
  isInitialized_ = false;
}

void RecorderAdapterNode::init(size_t bufferSize, int channelCount) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (isInitialized_ || context == nullptr) {
    return;
  }

  channelCount_ = channelCount;

  buff_.resize(channelCount_);

  for (size_t i = 0; i < channelCount_; ++i) {
    buff_[i] = std::make_shared<CircularOverflowableAudioArray>(bufferSize);
  }

  // This assumes that the sample rate is the same in audio context and recorder.
  // (recorder is not enforcing any sample rate on the system*). This means that only
  // channel mixing might be required. To do so, we create an output bus with
  // the desired channel count and will take advantage of the AudioBus sum method.
  //
  // * any allocations required by the recorder (including this method) are during recording start
  // or after, which means that audio context has already setup the system in 99% of sane cases.
  // But if we would like to support cases when context is created on the fly during recording,
  // we would need to add sample rate conversion as well or other weird bullshit like resampling
  // context output and not enforcing anything on the system output/input configuration.
  // A lot of words for a couple of lines of implementation :shrug:
  adapterOutputBus_ =
      std::make_shared<AudioBus>(RENDER_QUANTUM_SIZE, channelCount_, context->getSampleRate());
  isInitialized_ = true;
}

void RecorderAdapterNode::cleanup() {
  isInitialized_ = false;
  buff_.clear();
  adapterOutputBus_.reset();
}

std::shared_ptr<AudioBus> RecorderAdapterNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  if (!isInitialized_) {
    processingBus->zero();
    return processingBus;
  }

  readFrames(framesToProcess);

  processingBus->sum(adapterOutputBus_.get(), ChannelInterpretation::SPEAKERS);
  return processingBus;
}

void RecorderAdapterNode::readFrames(const size_t framesToRead) {
  adapterOutputBus_->zero();

  for (size_t channel = 0; channel < channelCount_; ++channel) {
    buff_[channel]->read(adapterOutputBus_->getChannel(channel)->getData(), framesToRead);
  }
}

} // namespace audioapi
