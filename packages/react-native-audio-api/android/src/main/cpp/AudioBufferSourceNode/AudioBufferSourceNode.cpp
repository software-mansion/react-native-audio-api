#include "AudioBufferSourceNode.h"
#include "AudioContext.h"

namespace audioapi {

AudioBufferSourceNode::AudioBufferSourceNode(AudioContext *context)
    : AudioScheduledSourceNode(context) {}

bool AudioBufferSourceNode::processAudio(float *audioData, int32_t numFrames) {
  // TODO implement
  return false;
}
} // namespace audioapi
