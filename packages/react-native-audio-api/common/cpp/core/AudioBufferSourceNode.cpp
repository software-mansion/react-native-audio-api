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

// Note: AudioBus copy method will use memcpy if the source buffer and system processing bus have same channel count,
// otherwise it will use the summing function taking care of up/down mixing.
void AudioBufferSourceNode::processNode(AudioBus* processingBus, int framesToProcess) {
  double time = context_->getCurrentTime();
  handlePlayback(time, framesToProcess);

  // No audio data to fill, zero the output and return.
  if (!isPlaying_ || !buffer_ || buffer_->getLength() == 0) {
    processingBus->zero();
    return;
  }

  // Easiest case, the buffer is the same length as the number of frames to process, just copy the data.
  if (framesToProcess == buffer_->getLength()) {
    processingBus->copy(*buffer_->bus_.get());

    if (!loop_) {
      isPlaying_ = false;
    }

    return;
  }

  // The buffer is longer than the number of frames to process.
  // We have to keep track of where we are in the buffer.
  if (framesToProcess < buffer_->getLength()) {
    int framesToCopy = std::min(framesToProcess, buffer_->getLength() - bufferIndex_);

    processingBus->copy(*buffer_->bus_.get(), bufferIndex_, framesToCopy);

    if (loop_) {
      bufferIndex_ = (bufferIndex_ + framesToCopy) % buffer_->getLength();
      return;
    }

    if (bufferIndex_ + framesToCopy == buffer_->getLength() - 1) {
      isPlaying_ = false;
      bufferIndex_ = 0;
    }

    return;
  }

  // processing bus is longer than the source buffer
  if (!loop_) {
    // If we don't loop the buffer, copy it once and zero the remaining processing bus frames.
    processingBus->copy(*buffer_->bus_.get());
    processingBus->zero(buffer_->getLength(), framesToProcess - buffer_->getLength());
    isPlaying_ = false;
    return;
  }

  // If we loop the buffer, we need to loop the buffer framesToProcess / bufferSize times
  // There might also be a remainder of frames to copy after the loop,
  // which will also carry over some buffer frames to the next render quantum.
  int processingBusPosition = 0;
  int bufferSize = buffer_->getLength();
  int remainingFrames = framesToProcess - framesToProcess / bufferSize;

  // Do we have some frames left in the buffer from the previous render quantum,
  // if yes copy them over and reset the buffer position.
  if (bufferIndex_ > 0) {
    processingBus->copy(*buffer_->bus_.get(), 0, bufferIndex_);
    processingBusPosition += bufferIndex_;
    bufferIndex_ = 0;
  }

  // Copy the entire buffer n times to the processing bus.
  while (processingBusPosition + bufferSize <= framesToProcess) {
    processingBus->copy(*buffer_->bus_.get());
    processingBusPosition += bufferSize;
  }

  // Fill in the remaining frames from the processing buffer and update buffer index for next render quantum.
  if (remainingFrames > 0) {
    processingBus->copy(*buffer_->bus_.get(), 0, processingBusPosition, remainingFrames);
    bufferIndex_ = remainingFrames;
  }
}

} // namespace audioapi
