
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <type_traits>

namespace audioapi {

RecorderAdapterNode::RecorderAdapterNode(BaseAudioContext *context) noexcept(
    std::is_nothrow_constructible<AudioNode, BaseAudioContext *>::value)
    : AudioNode(context) {
  // It should be marked as initialized only after it is connected to the
  // recorder. Internall buffer size is based on the recorder's buffer length.
  isInitialized_ = false;
}

void RecorderAdapterNode::init(size_t bufferSize) {
  if (isInitialized_) {
    throw std::runtime_error(
        "RecorderAdapterNode should not be initialized more than once. Just create a new instance.");
  }
  isInitialized_ = true;
  buff_ = std::make_shared<CircularOverflowableAudioArray>(bufferSize);
}

std::shared_ptr<AudioBus> RecorderAdapterNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  float *outputChannel = processingBus->getChannel(0)->getData();
  readFrames(outputChannel, framesToProcess);

  for (int i = 1; i < processingBus->getNumberOfChannels(); i++) {
    processingBus->getChannel(i)->copy(
        processingBus->getChannel(0), 0, framesToProcess);
  }

  return processingBus;
}

void RecorderAdapterNode::readFrames(float *output, const size_t framesToRead) {
  size_t readFrames = buff_->read(output, framesToRead);

  if (readFrames < framesToRead) {
    // Fill the rest with silence
    std::memset(
        output + readFrames, 0, (framesToRead - readFrames) * sizeof(float));
  }
}

} // namespace audioapi
