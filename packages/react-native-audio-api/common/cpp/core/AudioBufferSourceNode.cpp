#include "AudioBufferSourceNode.h"
#include "BaseAudioContext.h"

namespace audioapi {

AudioBufferSourceNode::AudioBufferSourceNode(BaseAudioContext *context)
    : AudioScheduledSourceNode(context), loop_(false), bufferIndex_(0) {
  // numberOfInputs_ = 0;
  // buffer_ = std::shared_ptr<AudioBuffer>(nullptr);
}

bool AudioBufferSourceNode::getLoop() const {
  // return loop_;
}

std::shared_ptr<AudioBuffer> AudioBufferSourceNode::getBuffer() const {
  // return buffer_;
}

void AudioBufferSourceNode::setLoop(bool loop) {
  // loop_ = loop;
}

void AudioBufferSourceNode::setBuffer(
    const std::shared_ptr<AudioBuffer> &buffer) {
  // if (!buffer) {
  //   buffer_ = std::shared_ptr<AudioBuffer>(nullptr);
  //   return;
  // }

  // buffer_ = buffer->mix(channelCount_);
}

// bool AudioBufferSourceNode::processAudio(float *audioData, int32_t numFrames) {
//   if (!isPlaying_ || !buffer_) {
//     return false;
//   } else {
//     for (int i = 0; i < numFrames; ++i) {
//       for (int j = 0; j < channelCount_; j++) {
//         audioData[i * channelCount_ + j] =
//             buffer_->getChannelData(j)[bufferIndex_];
//       }

//       bufferIndex_++;

//       if (bufferIndex_ >= buffer_->getLength()) {
//         if (loop_) {
//           bufferIndex_ = 0;
//         } else {
//           isPlaying_ = false;
//           break;
//         }
//       }
//     }

//     return true;
//   }
// }
} // namespace audioapi
