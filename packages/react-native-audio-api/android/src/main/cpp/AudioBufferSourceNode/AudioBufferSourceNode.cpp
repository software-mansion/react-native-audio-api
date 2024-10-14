#include "AudioBufferSourceNode.h"
#include "AudioContext.h"

namespace audioapi {

AudioBufferSourceNode::AudioBufferSourceNode(AudioContext *context)
    : AudioScheduledSourceNode(context), loop_(false), bufferIndex_(0) {
  numberOfInputs_ = 0;
  buffer_ = std::nullopt;
}

bool AudioBufferSourceNode::getLoop() const {
  return loop_;
}

std::shared_ptr<AudioBuffer> AudioBufferSourceNode::getBuffer() const {
    if (!buffer_.has_value()) {
        throw std::runtime_error("Buffer is not set");
    }

    return buffer_.value();
}

void AudioBufferSourceNode::setLoop(bool loop) {
  loop_ = loop;
}

void AudioBufferSourceNode::setBuffer(
    const std::shared_ptr<AudioBuffer> &buffer) {
  buffer_ = buffer->mix(channelCount_);
}

bool AudioBufferSourceNode::processAudio(float *audioData, int32_t numFrames) {
  if (!isPlaying_ || !buffer_.has_value()) {
    return false;
  } else {
    for (int i = 0; i < numFrames; ++i) {
      for (int j = 0; j < channelCount_; j++) {
        audioData[i * channelCount_ + j] =
            buffer_.value()->getChannelData(j)[bufferIndex_];
      }

      bufferIndex_++;

      if (bufferIndex_ >= buffer_.value()->getLength()) {
        if (loop_) {
          bufferIndex_ = 0;
        } else {
          isPlaying_ = false;
          break;
        }
      }
    }

    return true;
  }
}
} // namespace audioapi
