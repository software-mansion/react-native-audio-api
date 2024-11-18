#include "AudioBus.h"
#include "AudioArray.h"
#include "BaseAudioContext.h"
#include "AudioBufferSourceNode.h"

namespace audioapi {

AudioBufferSourceNode::AudioBufferSourceNode(BaseAudioContext *context)
    : AudioScheduledSourceNode(context), loop_(false), bufferIndex_(0) {
  numberOfInputs_ = 0;
  buffer_ = std::shared_ptr<AudioBuffer>(nullptr);
}

bool AudioBufferSourceNode::getLoop() const {
  return loop_;
}

std::shared_ptr<AudioBuffer> AudioBufferSourceNode::getBuffer() const {
  return buffer_;
}

void AudioBufferSourceNode::setLoop(bool loop) {
  loop_ = loop;
}

void AudioBufferSourceNode::setBuffer(
    const std::shared_ptr<AudioBuffer> &buffer) {

  if (!buffer) {
    buffer_ = std::shared_ptr<AudioBuffer>(nullptr);
    return;
  }

  buffer_ = buffer;
}

void AudioBufferSourceNode::processNode(AudioBus* processingBus, int framesToProcess) {
  double time = context_->getCurrentTime();
  handlePlayback(time, framesToProcess);

  if (!isPlaying_ || !buffer_) {
    processingBus->zero();
    return;
  }

  /*
  TODO: tomorrow:
  1. Add zeroRange, copyRange and sumRange to AudioArray and AudioBus
  2. Implement the following pseudo code:

      int framesToCopy = std::min(framesToProcess, buffer_->getLength() - bufferIndex_);
      int framesToFill = framesToProcess - framesToCopy;

      processingBus->sumRange(buffer_->getAudioBus() + bufferIndex_, 0, framesToCopy);

      bufferIndex_ += framesToCopy;

      if (bufferIndex_ >= buffer_->getLength()) {
        bufferIndex_ -= buffer_->getLength();
      }

      if (framesToFill > 0) {
        if (loop_) {
          processingBus->sumRange(buffer_->getAudioBus(), framesToCopy, framesToFill);
        } else {
          processingBus->zeroRange(framesToCopy, framesToFill);
          isPlaying_ = false;
          bufferIndex_ = 0;
        }
      }
*/
}

} // namespace audioapi
