
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <type_traits>

namespace audioapi {

RecorderAdapterNode::RecorderAdapterNode(BaseAudioContext *context) noexcept(
    std::is_nothrow_constructible<
        AudioScheduledSourceNode,
        BaseAudioContext *>::value)
    : AudioScheduledSourceNode(context), recorder_(nullptr) {}

void RecorderAdapterNode::setRecorder(
    const std::shared_ptr<AudioRecorder> &recorder) {
  recorder_ = recorder;
  isInitialized_ = true;
}

void RecorderAdapterNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;
  updatePlaybackInfo(processingBus, framesToProcess, startOffset, offsetLength);
  if (!isPlaying() && !isStopScheduled()) {
    processingBus->zero();
    return;
  }

  if (recorder_ == nullptr) {
    processingBus->zero();
    return;
  }

  float *outputChannel = processingBus->getChannel(0)->getData();

  recorder_->readFrames(outputChannel, framesToProcess);

  for (int i = 1; i < processingBus->getNumberOfChannels(); i++) {
    processingBus->getChannel(i)->copy(
        processingBus->getChannel(0), 0, framesToProcess);
  }

  handleStopScheduled();
}

} // namespace audioapi
